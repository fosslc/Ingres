/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwsel2.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select Page 2) 
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 01-Mars-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 21-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 01-Apr-2005 (komve01)
**    Bug #114200/ Issue #14052946. Multiple columns can be selected for export.
**	  Added code to iterate through all selected column names and add 
**    them to the results list.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqlwpsht.h"
#include "sqlepsht.h"
#include "sqlwsel2.h"
#include "edlssele.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardSelect2, CPropertyPage)

CuDlgPropertyPageSqlWizardSelect2::CuDlgPropertyPageSqlWizardSelect2() : CPropertyPage(CuDlgPropertyPageSqlWizardSelect2::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardSelect2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_strCurrentTable = _T("");
	m_bSpecifyColumn  = TRUE;
	m_bitmap.SetBitmpapId (IDB_SQL_SELECT2);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardSelect2::~CuDlgPropertyPageSqlWizardSelect2()
{
}

BOOL CuDlgPropertyPageSqlWizardSelect2::ExistColumnExpression()
{
	int i, nCount = m_cListBoxResultColumn.GetCount();
	CaSqlWizardDataField* pData = NULL;
	for (i=0; i<nCount; i++)
	{
		pData = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData(i);
		if (pData->m_iType == CaSqlWizardDataField::COLUMN_AGGREGATE)
			return TRUE;
	}
	return FALSE;
}


void CuDlgPropertyPageSqlWizardSelect2::EnableButtons()
{
	BOOL bEnableUp = FALSE;
	BOOL bEnableDown = FALSE;
	BOOL bEnableDel = FALSE;
	BOOL bEnableAdd = FALSE;

	int nSel, nCount = m_cListBoxResultColumn.GetCount();
	int nSelCount = (nCount == 0)? 0: m_cListBoxResultColumn.GetSelCount();

	if (nCount > 0 && nSelCount == 1)
	{
		nSel = m_cListBoxResultColumn.GetCurSel();
		if (nSel != LB_ERR)
		{
			bEnableUp = (nSel > 0 )? TRUE: FALSE;
			bEnableDown  = (nSel < (nCount-1))? TRUE: FALSE;
		}
	}
	bEnableDel = (nSelCount > 0)? TRUE: FALSE;
	m_cButtonUp.EnableWindow (bEnableUp);
	m_cButtonDown.EnableWindow (bEnableDown);
	m_cButtonDel.EnableWindow (bEnableDel);

	//
	// Button Add (>>)
	CString strText;
	m_cEditExpression.GetWindowText (strText);
	strText.TrimLeft();
	strText.TrimRight();
	if (strText.IsEmpty())
	{
		nSelCount = m_cListBoxColumn.GetSelCount();
		bEnableAdd = (nSelCount > 0) ? TRUE: FALSE;
		m_cButtonAdd.EnableWindow (bEnableAdd);
	}
	else
	{
		m_cButtonAdd.EnableWindow (TRUE);
	}
}

void CuDlgPropertyPageSqlWizardSelect2::EnableWizardButtons()
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();

	if (m_bSpecifyColumn)
	{
		if (m_cListBoxResultColumn.GetCount() == 0)
			pParent->SetWizardButtons(PSWIZB_BACK);
		else
			pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	}
	else
		pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlWizardSelect2::CleanComboTable()
{
	m_cComboTable.ResetContent();
}

void CuDlgPropertyPageSqlWizardSelect2::CleanListResultColumns()
{
	int i, nCount = m_cListBoxResultColumn.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaSqlWizardDataField* pObject = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData (i);
		if (pObject)
			delete pObject;
		m_cListBoxResultColumn.SetItemData (i, (DWORD)0);
	}
	m_cListBoxResultColumn.ResetContent();
}


void CuDlgPropertyPageSqlWizardSelect2::UpdateDisplayResultColumns (BOOL bMultipleTable)
{
	CWaitCursor doWaitCursor;
	CTypedPtrList<CObList, CaSqlWizardDataField*> listColumn;
	CaSqlWizardDataField* pObject = NULL;
	CString strItem;
	int i, nCount = m_cListBoxResultColumn.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaSqlWizardDataField* pObject = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData (i);
		if (pObject)
			listColumn.AddTail (pObject);
	}
	m_cListBoxResultColumn.ResetContent();
	POSITION pos = listColumn.GetHeadPosition();
	while (pos != NULL)
	{
		pObject = listColumn.GetNext(pos);
		if (bMultipleTable && pObject->m_iType == CaSqlWizardDataField::COLUMN_NORMAL)
			strItem = pObject->FormatString4Display();
		else
			strItem = pObject->m_strColumn;
		i = m_cListBoxResultColumn.AddString (strItem);
		if (i != LB_ERR)
			m_cListBoxResultColumn.SetItemData (i, (DWORD)pObject);
	}
}


BOOL CuDlgPropertyPageSqlWizardSelect2::MatchResultColumn (CaDBObject* pObj, LPCTSTR strColumn)
{
	int i, nCount = m_cListBoxResultColumn.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaSqlWizardDataField* pCol = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData (i);
		if (pCol)
		{
			if (pCol->GetTable().CompareNoCase (pObj->GetName()) == 0 &&
			    pCol->GetTableOwner().CompareNoCase (pObj->GetOwner()) == 0 &&
			    pCol->GetColumn().CompareNoCase (strColumn) == 0)
				return TRUE;
		}
	}

	return FALSE;
}

void CuDlgPropertyPageSqlWizardSelect2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardSelect2)
	DDX_Control(pDX, IDC_IMAGE_SELECT, m_bitmap);
	DDX_Control(pDX, IDC_STATIC3, m_cStaticExpression);
	DDX_Control(pDX, IDC_STATIC2, m_cStaticSpecifyColumn);
	DDX_Control(pDX, IDC_STATIC1, m_cStaticResultColumn);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonAdd);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonExpression);
	DDX_Control(pDX, IDC_EDIT1, m_cEditExpression);
	DDX_Control(pDX, IDC_BUTTON6, m_cButtonDel);
	DDX_Control(pDX, IDC_BUTTON5, m_cButtonDown);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonUp);
	DDX_Control(pDX, IDC_LIST2, m_cListBoxResultColumn);
	DDX_Control(pDX, IDC_LIST1, m_cListBoxColumn);
	DDX_Control(pDX, IDC_COMBO1, m_cComboTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardSelect2, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardSelect2)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboTable)
	ON_LBN_DBLCLK(IDC_LIST1, OnDblclkListColumn)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeListResultColumn)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioSpecifyColumns)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioAllColumns)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonExpression)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON5, OnButtonDown)
	ON_BN_CLICKED(IDC_BUTTON6, OnButtonDelete)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeListColumn)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEditExpression)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardSelect2 message handlers

BOOL CuDlgPropertyPageSqlWizardSelect2::OnSetActive() 
{
	CWaitCursor doWaitCursor;
	CaDBObject* pObj = NULL;
	int idx, i, nCount;
	CString strItem;
	CaSqlWizardDataField* pData = NULL;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	CuListCtrlCheckBox& clistBoxTable = pParent->m_PageSelect1.m_cListTable;
	CuListCtrlCheckBox& clistBoxView  = pParent->m_PageSelect1.m_cListView;

	//
	// Clean the Combobox:
	CleanComboTable();
	//
	// Initialize the combobox of table:
	nCount = clistBoxTable.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (clistBoxTable.GetItemChecked(i))
		{
			pObj = (CaDBObject*)clistBoxTable.GetItemData(i);
#if defined (_DISPLAY_OWNERxITEM)
			AfxFormatString2 (strItem, IDS_OWNERxITEM, (LPCTSTR)pObj->GetOwner(), (LPCTSTR)pObj->GetItem());
#else
			strItem = pObj->GetName();
#endif
			idx = m_cComboTable.AddString (strItem);
			if (idx != CB_ERR)
			{
				m_cComboTable.SetItemData (idx, (DWORD)pObj); // Share the Objects of Select Step 1
			}
		}
	}
	nCount = clistBoxView.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (clistBoxView.GetItemChecked(i))
		{
			pObj = (CaDBObject*)clistBoxView.GetItemData(i);
#if defined (_DISPLAY_OWNERxITEM)
			AfxFormatString2 (strItem, IDS_OWNERxITEM, (LPCTSTR)pObj->GetOwner(), (LPCTSTR)pObj->GetItem());
#else
			strItem = pObj->GetName();
#endif
			idx = m_cComboTable.AddString (strItem);
			if (idx != CB_ERR )
			{
				m_cComboTable.SetItemData (idx, (DWORD)pObj); // Share the Objects of Select Step 1
			}
		}
	}
	//
	// Select the table if any:
	if (!m_strCurrentTable.IsEmpty())
	{
		idx = m_cComboTable.FindStringExact (-1, m_strCurrentTable);
		if (idx != CB_ERR)
			m_cComboTable.SetCurSel(idx);
	}
	//
	// Simulate the selection change of Combobox table:
	if (m_cComboTable.GetCurSel() == CB_ERR)
		m_cComboTable.SetCurSel(0);
	OnSelchangeComboTable();
	//
	// Update the display of the list of result columns:
	if (m_cComboTable.GetCount() > 1)
		UpdateDisplayResultColumns(TRUE);
	else
		UpdateDisplayResultColumns(FALSE);

	nCount = m_cListBoxResultColumn.GetCount();
	i = 0;
	while (i<nCount && nCount > 0)
	{
		pData = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData(i);
		if (pData && !pData->GetTable().IsEmpty())
		{
			BOOL bFound = FALSE;
			int nCBCount = m_cComboTable.GetCount();
			for (int k=0; k<nCBCount; k++)
			{
				CaDBObject* pTable = (CaDBObject*)m_cComboTable.GetItemData(k);
				if (pTable->GetName().CompareNoCase(pData->GetTable()) == 0 && 
				    pTable->GetOwner().CompareNoCase(pData->GetTableOwner()) == 0)
				{
					bFound = TRUE;
					break;
				}
			}

			if (!bFound)
			{
				delete pData;
				m_cListBoxResultColumn.DeleteString (i);
				nCount = m_cListBoxResultColumn.GetCount();
				continue;
			}
		}
		i++;
	}

	EnableButtons();
	EnableWizardButtons();

	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlWizardSelect2::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CPropertyPage::OnInitDialog();
	//
	// Check the radio Specify Columns:
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	//
	// Simulate the Check radio Specify Column:
	OnRadioSpecifyColumns();
	EnableButtons();
	EnableWizardButtons();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlWizardSelect2::OnDestroy() 
{
	CleanComboTable();
	CleanListResultColumns();
	SQLW_ListBoxColumnsClean (&m_cListBoxColumn);
	CPropertyPage::OnDestroy();
}

void CuDlgPropertyPageSqlWizardSelect2::OnSelchangeComboTable() 
{
	CWaitCursor doWaitCursor;
	CaDBObject* pObject = NULL;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	int nSel = m_cComboTable.GetCurSel();

	SQLW_ListBoxColumnsClean (&m_cListBoxColumn);
	if (nSel == CB_ERR)
		return;
	try
	{
		CaLLQueryInfo info(pParent->m_queryInfo);
		pObject = (CaDBObject*)m_cComboTable.GetItemData (nSel);
		ASSERT(pObject);
		if (!pObject)
			return;
		info.SetItem2(pObject->GetName(), pObject->GetOwner());
		if (pObject->GetObjectID() == OBT_TABLE)
			info.SetObjectType(OBT_TABLECOLUMN);
		else
			info.SetObjectType(OBT_VIEWCOLUMN);
		SQLW_ListBoxFillColumns (&m_cListBoxColumn, &info);
	}
	catch (CMemoryException* e)
	{
		e->Delete();
		theApp.OutOfMemoryMessage();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		// _T("Cannot query the columns of the selected table");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_COLUMN, MB_ICONEXCLAMATION|MB_OK);
	}
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnDblclkListColumn() 
{
	CWaitCursor doWaitCursor;
	CaDBObject* pObj = NULL;
	int idx, nSelTable, nSel = m_cListBoxColumn.GetCurSel();
	int nSelCount = m_cListBoxColumn.GetSelCount();
	int nLoop = 0;
	if(nSelCount == 0)
	{
		return;
	}
	if (nSel == LB_ERR)
		return;

	nSelTable = m_cComboTable.GetCurSel();
	if (nSelTable == CB_ERR)
		return;

	pObj = (CaDBObject*)m_cComboTable.GetItemData(nSelTable);
	if (!pObj)
		return;

	CString strItem;
	//Multiple columns might be selected, iterate through them.
	int *pSelItemsIndex = new int[nSelCount];
	if(!pSelItemsIndex)
	{
		return; //Failure
	}
	m_cListBoxColumn.GetSelItems(nSelCount,pSelItemsIndex);
	
	while(nLoop < nSelCount)
	{
		m_cListBoxColumn.GetText (pSelItemsIndex[nLoop], strItem);
		//
		// Check if column is already added:
		if (!MatchResultColumn(pObj, strItem))
		{
			CaSqlWizardDataField* pData = new CaSqlWizardDataField (CaSqlWizardDataField::COLUMN_NORMAL, strItem);
			pData->SetTable(pObj->GetName(), pObj->GetOwner(), pObj->GetObjectID());

			if (m_cComboTable.GetCount() > 1)
				idx = m_cListBoxResultColumn.AddString (pData->FormatString4Display());
			else
				idx = m_cListBoxResultColumn.AddString (strItem);
			if (idx != LB_ERR)
				m_cListBoxResultColumn.SetItemData (idx, (DWORD)pData);
		}
		nLoop++;
	}//Loop around the selected items and add them to the results list
	if(pSelItemsIndex)
	{
		delete []pSelItemsIndex;
		pSelItemsIndex = NULL;//Good Practise
	}
	EnableButtons();
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnSelchangeListResultColumn() 
{
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnRadioSpecifyColumns() 
{
	m_bSpecifyColumn = TRUE;
	int nShow = SW_SHOW;

	m_cEditExpression.ShowWindow (nShow);
	m_cButtonDel.ShowWindow (nShow);
	m_cButtonDown.ShowWindow (nShow);
	m_cButtonUp.ShowWindow (nShow);
	m_cListBoxResultColumn.ShowWindow (nShow);
	m_cListBoxColumn.ShowWindow (nShow);
	m_cComboTable.ShowWindow (nShow);
	m_cStaticExpression.ShowWindow (nShow);
	m_cStaticSpecifyColumn.ShowWindow (nShow);
	m_cStaticResultColumn.ShowWindow (nShow);
	m_cButtonAdd.ShowWindow (nShow);
	m_cButtonExpression.ShowWindow (nShow);
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnRadioAllColumns() 
{
	m_bSpecifyColumn = FALSE;
	int nShow = SW_HIDE;

	m_cEditExpression.ShowWindow (nShow);
	m_cButtonDel.ShowWindow (nShow);
	m_cButtonDown.ShowWindow (nShow);
	m_cButtonUp.ShowWindow (nShow);
	m_cListBoxResultColumn.ShowWindow (nShow);
	m_cListBoxColumn.ShowWindow (nShow);
	m_cComboTable.ShowWindow (nShow);
	m_cStaticExpression.ShowWindow (nShow);
	m_cStaticSpecifyColumn.ShowWindow (nShow);
	m_cStaticResultColumn.ShowWindow (nShow);
	m_cButtonAdd.ShowWindow (nShow);
	m_cButtonExpression.ShowWindow (nShow);
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnButtonExpression() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CxDlgPropertySheetSqlExpressionWizard wizdlg;
	wizdlg.m_queryInfo = pParent->m_queryInfo;
	wizdlg.m_nFamilyID    = FF_AGGRFUNCTIONS;
	wizdlg.m_nAggType     = IDC_RADIO1;
	wizdlg.m_nCompare     = IDC_RADIO1;
	wizdlg.m_nIn_notIn    = IDC_RADIO1;
	wizdlg.m_nSub_List    = IDC_RADIO1;
	//
	// Initialize Tables or Views from the checked items:
	try
	{
		CString strItem;
		CaDBObject* pObject = NULL;
		int nSel = -1;
		int i, nCount = m_cComboTable.GetCount();
		for (i=0; i<nCount; i++)
		{
			pObject = (CaDBObject*)m_cComboTable.GetItemData (i);
			CaDBObject* pNewObject = new CaDBObject(*pObject);
			pNewObject->SetObjectID(pObject->GetObjectID());
			wizdlg.m_listObject.AddTail (pNewObject);
		}
		int nResult = wizdlg.DoModal();
		if (nResult == IDCANCEL)
			return;
		
		CString strStatement;
		wizdlg.GetStatement (strStatement);
		if (strStatement.IsEmpty())
			return;
		m_cEditExpression.ReplaceSel (strStatement);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return;
	}
	catch(...)
	{
		// _T("Cannot initialize the SQL assistant.");
		AfxMessageBox (IDS_MSG_SQL_ASSISTANT, MB_ICONEXCLAMATION|MB_OK);
		return;
	}
}

void CuDlgPropertyPageSqlWizardSelect2::OnButtonAdd() 
{
	CWaitCursor doWaitCursor;
	OnDblclkListColumn();
}

void CuDlgPropertyPageSqlWizardSelect2::OnButtonUp() 
{
	m_cListBoxResultColumn.SetFocus();
	int nSelected = m_cListBoxResultColumn.GetCurSel();
	if (nSelected == LB_ERR || nSelected == 0)
		return;

	CString strCurrentItem;
	CString strUpItem;
	CaSqlWizardDataField* pDataUp = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData(nSelected-1);
	m_cListBoxResultColumn.GetText (nSelected-1, strUpItem);

	//
	// Delete the upper item, the current item becomes the upper item:
	if (m_cListBoxResultColumn.DeleteString (nSelected-1) != LB_ERR)
	{
		int idx = m_cListBoxResultColumn.InsertString (nSelected, strUpItem);
		if (idx != LB_ERR)
		{
			m_cListBoxResultColumn.SetItemData (idx, (DWORD)pDataUp);
			m_cListBoxResultColumn.SetCaretIndex(nSelected-1);
			m_cListBoxResultColumn.SetCurSel (nSelected-1);
		}
		else
		if (pDataUp)
			delete pDataUp;
	}
	m_cListBoxResultColumn.Invalidate();
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnButtonDown() 
{
	m_cListBoxResultColumn.SetFocus();
	int nSelected = m_cListBoxResultColumn.GetCurSel();
	if (nSelected == LB_ERR || nSelected == (m_cListBoxResultColumn.GetCount() -1))
		return;

	CString strDownItem;
	CaSqlWizardDataField* pDataDown = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData(nSelected+1);
	m_cListBoxResultColumn.GetText (nSelected+1, strDownItem);

	//
	// Delete the current selection +1, and insert it at the current selected item:
	if (m_cListBoxResultColumn.DeleteString (nSelected +1) != LB_ERR)
	{
		int idx = m_cListBoxResultColumn.InsertString (nSelected, strDownItem);
		if (idx != LB_ERR)
		{
			m_cListBoxResultColumn.SetItemData (idx, (DWORD)pDataDown);
			m_cListBoxResultColumn.SetCaretIndex(nSelected+1);
			m_cListBoxResultColumn.SetCurSel (nSelected +1);
		}
		else
		if (pDataDown)
			delete pDataDown;
	}
	m_cListBoxResultColumn.Invalidate();
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnButtonDelete() 
{
	CWaitCursor doWaitCursor;
	m_cListBoxResultColumn.SetFocus();
	CaSqlWizardDataField* pData = NULL;
	int i, nCount= m_cListBoxResultColumn.GetCount();
	i =0;
	while (i<nCount && nCount>0)
	{
		if (m_cListBoxResultColumn.GetSel (i) > 0)
		{
			pData = (CaSqlWizardDataField*)m_cListBoxResultColumn.GetItemData(i);
			if (pData)
				delete pData;
			m_cListBoxResultColumn.DeleteString (i);
			nCount= m_cListBoxResultColumn.GetCount();
			continue;
		}
		else
			i++;
	}
	EnableButtons();
	EnableWizardButtons();
}

BOOL CuDlgPropertyPageSqlWizardSelect2::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

LRESULT CuDlgPropertyPageSqlWizardSelect2::OnWizardBack() 
{
	int nSel = m_cComboTable.GetCurSel();
	if (nSel != CB_ERR)
		m_cComboTable.GetLBText (nSel, m_strCurrentTable);
	else
		m_strCurrentTable = _T("");
	return CPropertyPage::OnWizardBack();
}

LRESULT CuDlgPropertyPageSqlWizardSelect2::OnWizardNext() 
{
	int nSel = m_cComboTable.GetCurSel();
	if (nSel != CB_ERR)
		m_cComboTable.GetLBText (nSel, m_strCurrentTable);
	else
		m_strCurrentTable = _T("");
	return CPropertyPage::OnWizardNext();
}

void CuDlgPropertyPageSqlWizardSelect2::OnSelchangeListColumn() 
{
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnChangeEditExpression() 
{
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect2::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardSelect2::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_SEL2);
}

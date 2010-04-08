/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlwsel3.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Insert Page 2)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
** 02-Mar-2000 (schph01)
**    bug 104117 : retrieve the text on current selected item before deleted item.
** 02-Mar-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 08-May-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up the warnings.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "sqlwpsht.h"
#include "sqlepsht.h"
#include "sqlwsel3.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardSelect3, CPropertyPage)

CuDlgPropertyPageSqlWizardSelect3::CuDlgPropertyPageSqlWizardSelect3() : CPropertyPage(CuDlgPropertyPageSqlWizardSelect3::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardSelect3)
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_SQL_SELECT3);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardSelect3::~CuDlgPropertyPageSqlWizardSelect3()
{
}

void CuDlgPropertyPageSqlWizardSelect3::CleanColumnOrder()
{
	m_cListCtrl.DeleteAllItems();
}

void CuDlgPropertyPageSqlWizardSelect3::EnableButtons()
{
	int  nSel = -1, nCount;
	BOOL bEnableGUp   = FALSE;
	BOOL bEnableGDown = FALSE;
	BOOL bEnableOUp   = FALSE;
	BOOL bEnableODown = FALSE;
	BOOL bEnableAdd   = FALSE;
	BOOL bEnableDel   = FALSE;

	//
	// ListBox Group Order:
	nCount = m_cListBoxGroupBy.GetCount();
	if (m_cCheckGroupBy.GetCheck () && nCount > 0)
	{
		nSel = m_cListBoxGroupBy.GetCurSel();
		if (nSel != LB_ERR)
		{
			bEnableGUp = (nSel > 0 )? TRUE: FALSE;
			bEnableGDown  = (nSel < (nCount-1))? TRUE: FALSE;
		}
		m_cButtonGUp.EnableWindow(bEnableGUp);
		m_cButtonGDown.EnableWindow(bEnableGDown);
	}
	//
	// CListBox Sorted Order:
	nSel = -1;
	nCount = m_cListCtrl.GetItemCount();
	nSel = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
	if (nSel != -1)
	{
		bEnableOUp = (nSel > 0 )? TRUE: FALSE;
		bEnableODown  = (nSel < (nCount-1))? TRUE: FALSE;
		bEnableDel  = TRUE;
	}
	m_cButtonOUp.EnableWindow(bEnableOUp);
	m_cButtonODown.EnableWindow(bEnableODown);
	//
	// Button Add/Remove the sorted order columns:
	m_cButtonRemoveSortOrder.EnableWindow(bEnableDel);
	nSel = m_cListBoxOrderBy.GetCurSel();
	if (nSel != LB_ERR)
		bEnableAdd = TRUE;
	m_cButtonAddSortOrder.EnableWindow(bEnableAdd);
}


void CuDlgPropertyPageSqlWizardSelect3::InitializeListBoxAllColumns(CListBox* pListBox)
{
	int i, idx, nCount;
	CString strItem;
	CaDBObject* pTable;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CComboBox& comboTable = pParent->m_PageSelect2.m_cComboTable;

	//
	// All columns are added into the list:
	nCount = comboTable.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaLLQueryInfo info(pParent->m_queryInfo);
		pTable = (CaDBObject*)comboTable.GetItemData(i);
		if (pTable->GetObjectID() == OBT_TABLE)
			info.SetObjectType(OBT_TABLECOLUMN);
		else
			info.SetObjectType(OBT_VIEWCOLUMN);
		info.SetItem2(pTable->GetName(), pTable->GetOwner());

		CTypedPtrList<CObList, CaDBObject*> listObject;
		BOOL bOk = theApp.INGRESII_QueryObject (&info, listObject);
		if (!bOk)
			return;

		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CaDBObject* pObj = listObject.GetNext(pos);
			if (comboTable.GetCount() > 1)
			{
				CString strOwner = pTable->GetOwner();
				CString strName  = pTable->GetName();
				CString strCol   = pObj->GetName();

				if (strOwner.FindOneOf(_T(".\"")) != -1)
					strOwner = INGRESII_llQuoteIfNeeded(pTable->GetOwner());
				if (strName.FindOneOf(_T(".\"")) != -1)
					strName = INGRESII_llQuoteIfNeeded(pTable->GetName());
				if (strCol.FindOneOf(_T(".\"")) != -1)
					strCol = INGRESII_llQuoteIfNeeded(pObj->GetName());

				strItem.Format (_T("%s.%s.%s"), (LPCTSTR)strOwner, (LPCTSTR)strName, (LPCTSTR)strCol);
			}
			else
				strItem = pObj->GetName();

			idx = pListBox->AddString ((LPCTSTR)strItem);
			if (idx != LB_ERR)
			{
				CaSqlWizardDataField* pData = new CaSqlWizardDataField (CaSqlWizardDataField::COLUMN_NORMAL, pObj->GetName());
				pData->SetTable(pTable->GetName(), pTable->GetOwner(), pTable->GetObjectID());
				pListBox->SetItemData(idx, (LPARAM)pData);
			}

			delete pObj;
		}
	}
}


void CuDlgPropertyPageSqlWizardSelect3::InitializeListBoxOrderColumn()
{
	int i, nCount,nSel;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CListBox& clistBoxResultColumn = pParent->m_PageSelect2.m_cListBoxResultColumn;
	CComboBox& comboTable = pParent->m_PageSelect2.m_cComboTable;
	CaSqlWizardDataField* pData = NULL;

	if (pParent->m_PageSelect2.IsSpecifyColumn())
	{
		CString strItem;
		nCount = clistBoxResultColumn.GetCount();
		for (i=0; i<nCount; i++)
		{
			clistBoxResultColumn.GetText (i, strItem);
			pData = (CaSqlWizardDataField*)clistBoxResultColumn.GetItemData(i);
			if (comboTable.GetCount() > 1 && pData->m_iType == CaSqlWizardDataField::COLUMN_NORMAL)
				strItem = pData->FormatString4Display();
			else
				strItem = pData->GetColumn();
			nSel = m_cListBoxOrderBy.AddString (strItem);
			if (nSel != LB_ERR || nSel != LB_ERRSPACE)
			{
				CaSqlWizardDataField* pa = new CaSqlWizardDataField(*pData);
				m_cListBoxOrderBy.SetItemData(nSel, (LPARAM)pa);
			}
		}
	}
	else
	{
		//
		// All columns are added into the list:
		InitializeListBoxAllColumns(&m_cListBoxOrderBy);
	}
}


void CuDlgPropertyPageSqlWizardSelect3::InitializeListBoxGroupByColumn()
{
	int i, nCount,nSel;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CListBox& clistBoxResultColumn = pParent->m_PageSelect2.m_cListBoxResultColumn;
	CComboBox& comboTable = pParent->m_PageSelect2.m_cComboTable;
	CaSqlWizardDataField* pData = NULL;

	nCount = clistBoxResultColumn.GetCount();
	m_cListBoxGroupBy.ResetContent();
	if (pParent->m_PageSelect2.IsSpecifyColumn())
	{
		CString strItem;
		for (i=0; i<nCount; i++)
		{
			pData = (CaSqlWizardDataField*)clistBoxResultColumn.GetItemData(i);
			if (pData && pData->m_iType == CaSqlWizardDataField::COLUMN_NORMAL)
			{
				if (comboTable.GetCount() > 1)
					strItem = pData->FormatString4Display();
				else
					strItem = pData->m_strColumn;
				nSel = m_cListBoxGroupBy.AddString (strItem);
				if (nSel != LB_ERR || nSel != LB_ERRSPACE)
				{
					CaSqlWizardDataField* pa = new CaSqlWizardDataField( *pData );
					m_cListBoxGroupBy.SetItemData(nSel, (LPARAM)pa);
				}
			}
		}
	}
	else
	{
		//
		// All columns are added into the list:
		InitializeListBoxAllColumns(&m_cListBoxGroupBy);
	}
}

void CuDlgPropertyPageSqlWizardSelect3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardSelect3)
	DDX_Control(pDX, IDC_IMAGE_FINISH, m_bitmap);
	DDX_Control(pDX, IDC_BUTTON11, m_cButtonODown);
	DDX_Control(pDX, IDC_BUTTON10, m_cButtonOUp);
	DDX_Control(pDX, IDC_BUTTON7, m_cButtonRemoveSortOrder);
	DDX_Control(pDX, IDC_BUTTON6, m_cButtonAddSortOrder);
	DDX_Control(pDX, IDC_BUTTON9, m_cButtonHaving);
	DDX_Control(pDX, IDC_BUTTON5, m_cButtonGDown);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonGUp);
	DDX_Control(pDX, IDC_STATIC3, m_cStaticHaving);
	DDX_Control(pDX, IDC_LIST2, m_cListBoxOrderBy);
	DDX_Control(pDX, IDC_LIST1, m_cListBoxGroupBy);
	DDX_Control(pDX, IDC_EDIT4, m_cEditHaving);
	DDX_Control(pDX, IDC_EDIT2, m_cEditUnion);
	DDX_Control(pDX, IDC_EDIT1, m_cEditWhere);
	DDX_Control(pDX, IDC_CHECK2, m_cCheckGroupBy);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckDistinct);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardSelect3, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardSelect3)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonWhere)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonGroupUp)
	ON_BN_CLICKED(IDC_BUTTON5, OnButtonGroupDown)
	ON_BN_CLICKED(IDC_BUTTON9, OnButtonHaving)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonUnion)
	ON_BN_CLICKED(IDC_BUTTON6, OnButtonOrderAdd)
	ON_BN_CLICKED(IDC_BUTTON7, OnButtonOrderRemove)
	ON_BN_CLICKED(IDC_BUTTON10, OnButtonOrderUp)
	ON_BN_CLICKED(IDC_BUTTON11, OnButtonOrderDown)
	ON_BN_CLICKED(IDC_CHECK2, OnCheckGroupBy)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeListGroupBy)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeListColumn)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST3, OnItemchangedListOrderColumn)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardSelect3 message handlers

BOOL CuDlgPropertyPageSqlWizardSelect3::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CPropertyPage::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST3, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.SetFullRowSelected (TRUE);

	CString strItem;
	LVCOLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[2] =
	{
		{IDS_HEADER_COLUMN,      120, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_SORT_TYPE,   120, LVCFMT_LEFT, FALSE}
	};
	memset (&lvcolumn, 0, sizeof (LVCOLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<2; i++)
	{
		strItem.LoadString(lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strItem;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn); 
	}
	OnCheckGroupBy();
	EnableButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlWizardSelect3::OnDestroy() 
{
	CleanListBoxGroupBy();
	CleanListBoxOrderBy();
	CleanColumnOrder();
	m_ImageList.DeleteImageList();
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardSelect3::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();

	return CPropertyPage::OnKillActive();
}

BOOL CuDlgPropertyPageSqlWizardSelect3::OnSetActive() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	try
	{
		CComboBox& ccomboBoxTable = pParent->m_PageSelect2.m_cComboTable;
		CleanListBoxGroupBy();
		//
		// Clean the Combobox:
		CleanListBoxOrderBy();
		//
		// Initialize the listbox of Order by:
		InitializeListBoxOrderColumn();
		//
		// Update the order by columns.
		// Remove the colunms in the list of "order by layout" if those columns are not existed any more.
		BOOL* pBExist = NULL;
		int nCount = m_cListCtrl.GetItemCount();
		if (nCount > 0)
		{
			pBExist = new BOOL [nCount];
		}
		for (int i= 0; i<nCount; i++)
		{
			CaSqlWizardDataFieldOrder* pObj = (CaSqlWizardDataFieldOrder*)m_cListCtrl.GetItemData(i);
			pBExist[i]  = FALSE;
			if (pObj)
			{
				int k, nLBCount = m_cListBoxOrderBy.GetCount();
				for (k=0; k<nLBCount; k++)
				{
					CaSqlWizardDataField* pF = (CaSqlWizardDataField*)m_cListBoxOrderBy.GetItemData(k);
					if (ccomboBoxTable.GetCount() > 1)
						pObj->m_bMultiTablesSelected = TRUE;
					else
						pObj->m_bMultiTablesSelected = FALSE;
					if (pF->GetTable().CompareNoCase(pObj->GetTable()) == 0 &&
					    pF->GetTableOwner().CompareNoCase(pObj->GetTableOwner()) == 0 &&
					    pF->GetColumn().CompareNoCase(pObj->GetColumn()) == 0 )
					{
						pBExist[i] = TRUE;
						break;
					}
				}
			}
		}
		
		if (nCount > 0 && pBExist)
		{
			int i = nCount-1;
			while (i>=0)
			{
				if (!pBExist[i])
					m_cListCtrl.DeleteItem (i);
				i--;
			}
			delete pBExist;
		}
		m_cListCtrl.UpdateDisplay();

		//
		// Initialize the listbox of group by:
		InitializeListBoxGroupByColumn();

		//
		// If exist a column expression then check the group by:
		if (pParent->m_PageSelect2.ExistColumnExpression())
		{
			m_cCheckGroupBy.SetCheck (1);
			OnCheckGroupBy();
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (_T("Internal error: \nCuDlgPropertyPageSqlWizardSelect3.OnSetActive()"), MB_ICONEXCLAMATION|MB_OK);
	}
	EnableButtons();
	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlWizardSelect3::OnWizardFinish() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	BOOL bDistinct = IsDlgButtonChecked (IDC_CHECK1);
	//
	// Generate the Statement:
	try
	{
		POSITION pos = NULL;
		CString strInsertStatement = _T("");
		CString strStatement = _T("");
		int i, nCount;
		CString strText;
		CString strItem;
		CString strItemOwner;
		CListBox&  clistBoxResultColumn = pParent->m_PageSelect2.m_cListBoxResultColumn;
		CComboBox& ccomboBoxTable = pParent->m_PageSelect2.m_cComboTable;
		BOOL bUsedUnion = FALSE;
		BOOL bSpecifyColumn = pParent->m_PageSelect2.IsSpecifyColumn();
		
		//
		// Generate the SELECT [all|distinct] x, y, z | SELECT [all|distinct] *:
		BOOL bOne = TRUE;
		if (bSpecifyColumn)
		{
			if (bDistinct)
				strStatement.Format (_T("%s %s "), cstrSelect, cstrDistinct);
			else
				strStatement.Format (_T("%s "), cstrSelect);
			nCount = clistBoxResultColumn.GetCount();
			for (i=0; i<nCount; i++)
			{
				CaSqlWizardDataField* pData = NULL;
				pData = (CaSqlWizardDataField*)clistBoxResultColumn.GetItemData( i );
				if(!pData)
					continue;
				if (pData->m_iType == CaSqlWizardDataField::COLUMN_NORMAL)
				{
					if ( ccomboBoxTable.GetCount() > 1 )
						strItem = pData->FormatString4SQL();
					else
						strItem = INGRESII_llQuoteIfNeeded(pData->GetColumn());
				}
				else
					strItem = pData->GetColumn();

				if (!bOne)
					strStatement += _T(", ");
				strStatement += strItem;
				bOne = FALSE;
			}
		}
		else
		{
			if (bDistinct)
				strStatement.Format (_T("%s * %s "), cstrSelect, cstrDistinct);
			else
				strStatement.Format (_T("%s * "), cstrSelect);
		}
		//
		// Generate the list of tables: FROM t1, t2, t3, ...
		strStatement += _T(" ");
		strStatement += cstrFrom;
		strStatement += _T(" ");
		bOne = TRUE;
		nCount = ccomboBoxTable.GetCount();
		for (i=0; i<nCount; i++)
		{
			CaDBObject* pObject = NULL;
			pObject = (CaDBObject*)ccomboBoxTable.GetItemData (i);
			if (!pObject)
				continue;

			if (!bOne)
				strStatement += _T(", ");
			if (pObject->GetOwner().IsEmpty())
				strStatement += INGRESII_llQuoteIfNeeded(pObject->GetItem()); // no owner in Object Name
			else
			{
				strStatement += INGRESII_llQuoteIfNeeded(pObject->GetOwner()); // owner in Object Name
				strStatement += _T(".");
				strStatement += INGRESII_llQuoteIfNeeded(pObject->GetItem());
			}
			bOne = FALSE;
		}
		//
		// Generate the WHERE CLAUSE:
		m_cEditWhere.GetWindowText (strText);
		strText.TrimLeft();
		strText.TrimRight();
		if (!strText.IsEmpty())
		{
			strStatement += _T(" ");
			strStatement += cstrWhere;
			strStatement += _T(" ");
			strStatement += strText;
		}
		//
		// Generate the GROUP BY CLAUSE:
		if (IsDlgButtonChecked (IDC_CHECK2))
		{
			strStatement += _T(" ");
			strStatement += cstrGroupBy;
			strStatement += _T(" ");
			bOne = TRUE;
			nCount = m_cListBoxGroupBy.GetCount();
			CaSqlWizardDataField* pData = NULL;
			for (i=0; i<nCount; i++)
			{
				m_cListBoxGroupBy.GetText (i, strItem);
				pData = (CaSqlWizardDataField*)m_cListBoxGroupBy.GetItemData(i);
				if (!pData)
					continue;

				if (!bOne)
					strStatement += _T(", ");
				bOne = FALSE;
				if ( ccomboBoxTable.GetCount() > 1 )
					strStatement += pData->FormatString4SQL();
				else
					strStatement += INGRESII_llQuoteIfNeeded(pData->GetColumn());
			}
			m_cEditHaving.GetWindowText (strText);
			strText.TrimLeft();
			strText.TrimRight();
			if (!strText.IsEmpty())
			{
				strStatement += _T(" ");
				strStatement += cstrHaving;
				strStatement += _T(" ");
				strStatement += strText;
			}
		}
		//
		// Generate the Union clause:
		m_cEditUnion.GetWindowText (strText);
		strText.TrimLeft();
		strText.TrimRight();
		if (!strText.IsEmpty())
		{
			bUsedUnion = TRUE;
			strStatement += _T(" ");
			strStatement += cstrUnion;
			strStatement += _T(" ");
			strStatement += strText;
		}

		//
		// Generate the ORDER BY:
		nCount = m_cListCtrl.GetItemCount();
		if (nCount > 0)
		{
			strStatement += _T(" ");
			strStatement += cstrOrderBy;
			strStatement += _T(" ");
			if (!bUsedUnion)
			{
				CaSqlWizardDataFieldOrder* pItemOrder = NULL;
				bOne = TRUE;
				for (i=0; i<nCount; i++)
				{
					pItemOrder = (CaSqlWizardDataFieldOrder*)m_cListCtrl.GetItemData(i);
					if (!pItemOrder)
						continue;
					strItem = m_cListCtrl.GetItemText (i, 0);
					strText = m_cListCtrl.GetItemText (i, 1);
					if (!bOne)
						strStatement += _T(", ");
					bOne = FALSE;

					if (pItemOrder->m_iType == CaSqlWizardDataFieldOrder::COLUMN_NORMAL)
					{
						// 
						// Order by column name:
						if ( ccomboBoxTable.GetCount() > 1 )
							strStatement += pItemOrder->FormatString4SQL();
						else
							strStatement += INGRESII_llQuoteIfNeeded(pItemOrder->GetColumn());

						strStatement += _T(" ");
						strStatement += strText;
					}
					else
					{
						//
						// Order by column position:
						int nIdx = clistBoxResultColumn.FindStringExact (-1, strItem);
						if (nIdx != LB_ERR)
						{
							//
							// 1 base index (nIdx +1):
							strItem.Format (_T("%d %s"), nIdx+1, (LPCTSTR)strText);
							strStatement += strItem;
						}
					}
				}
			}
			else
			{
				//
				// Use the column range instead of column name:
				bOne = TRUE;
				int nIdx = -1;
				for (i=0; i<nCount; i++)
				{
					strItem = m_cListCtrl.GetItemText (i, 0);
					nIdx = clistBoxResultColumn.FindStringExact (-1, strItem);
					if (nIdx == LB_ERR)
						continue;
					strText = m_cListCtrl.GetItemText (i, 1);
					if (!bOne)
						strStatement += _T(", ");
					bOne = FALSE;
					//
					// 1 base index (nIdx +1):
					strItem.Format (_T("%d %s"), nIdx+1, (LPCTSTR)strText);
					strStatement += strItem;
				}
			}
		}
		if (pParent->GetCategory() == CxDlgPropertySheetSqlWizard::SQLW_INSERT)
		{
			//
			// This sub-select statement is used by The Insert statement.
			// So, generate the insert statement:

			CString strListItem = _T("");
			CuCheckListBox& checkListBoxColumn = pParent->m_PageInsert1.m_cCheckListBoxColumn;
			CComboBox& comboTableInsert        = pParent->m_PageInsert1.m_cComboTable;
			nCount = checkListBoxColumn.GetCount();
			bOne   = TRUE;
			//
			// Get the column, skip the first item:
			for (i=1; i<nCount; i++)
			{
				if (!checkListBoxColumn.GetCheck(i))
					continue;
				checkListBoxColumn.GetText (i, strItem);
				if (!bOne)
					strListItem += _T(", ");
				strListItem += INGRESII_llQuoteIfNeeded(strItem);
				bOne = FALSE;
			}
			//
			// Get the Table:
			i = comboTableInsert.GetCurSel();
			if (i == CB_ERR)
			{
				pParent->SetStatement (NULL);
				return TRUE;
			}
			comboTableInsert.GetLBText (i, strItem);
			if (!strListItem.IsEmpty())
				strInsertStatement.Format (_T("%s %s %s (%s) "), cstrInsert, cstrInto, (LPCTSTR)strItem, (LPCTSTR)strListItem);
			else
				strInsertStatement.Format (_T("%s %s %s "), cstrInsert, cstrInto, (LPCTSTR)INGRESII_llQuoteIfNeeded(strItem));
		}
		if (!strInsertStatement.IsEmpty())
		{
			strInsertStatement += strStatement;
			pParent->SetStatement (strInsertStatement);
		}
		else
			pParent->SetStatement (strStatement);
		return CPropertyPage::OnWizardFinish();
	}
	catch (CMemoryException* e)
	{
		e->Delete();
		theApp.OutOfMemoryMessage();
	}
	catch (...)
	{
		//_T("Internal error: cannot generate the statement");
		AfxMessageBox (IDS_MSG_FAIL_2_GENERATE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
	}

	pParent->SetStatement (NULL);
	return FALSE;
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonWhere() 
{
	OnExpressionAssistant(_T('W'));
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonGroupUp() 
{
	m_cListBoxGroupBy.SetFocus();
	int nSelected = m_cListBoxGroupBy.GetCurSel();
	if (nSelected == LB_ERR || nSelected == 0)
		return;

	CString strUpItem;
	CaSqlWizardDataField* pDataUp = NULL;
	m_cListBoxGroupBy.GetText (nSelected-1, strUpItem);

	pDataUp = (CaSqlWizardDataField*)m_cListBoxGroupBy.GetItemData (nSelected-1);
	if (!pDataUp)
		return;
	if (m_cListBoxGroupBy.DeleteString (nSelected-1) != LB_ERR)
	{
		m_cListBoxGroupBy.InsertString (nSelected, strUpItem);
		m_cListBoxGroupBy.SetItemData  (nSelected, (LPARAM)pDataUp);
		m_cListBoxGroupBy.SetCaretIndex(nSelected-1);
		m_cListBoxGroupBy.SetCurSel (nSelected-1);
	}
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonGroupDown() 
{
	m_cListBoxGroupBy.SetFocus();
	int nSelected = m_cListBoxGroupBy.GetCurSel();
	if (nSelected == LB_ERR || nSelected == (m_cListBoxGroupBy.GetCount() -1))
		return;

	CString strDownItem;
	CaSqlWizardDataField* pDataDown = NULL;

	m_cListBoxGroupBy.GetText (nSelected, strDownItem);
	pDataDown = (CaSqlWizardDataField*)m_cListBoxGroupBy.GetItemData (nSelected);
	if (!pDataDown)
		return;

	if (m_cListBoxGroupBy.DeleteString (nSelected) != LB_ERR)
	{
		m_cListBoxGroupBy.InsertString (nSelected+1, strDownItem);
		m_cListBoxGroupBy.SetItemData  (nSelected+1, (LPARAM)pDataDown);
		m_cListBoxGroupBy.SetCaretIndex(nSelected+1);
		m_cListBoxGroupBy.SetCurSel (nSelected+1);
	}
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonHaving() 
{
	OnExpressionAssistant(_T('H'));
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonUnion() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	//
	// Select only mode:
	CxDlgPropertySheetSqlWizard sqlWizard (TRUE, this);
	sqlWizard.m_queryInfo = pParent->m_queryInfo;
	if (sqlWizard.DoModal() != IDCANCEL)
	{
		CString strStatement;
		sqlWizard.GetStatement (strStatement);
		strStatement.TrimLeft();
		strStatement.TrimRight();
		if (!strStatement.IsEmpty())
			m_cEditUnion.ReplaceSel(strStatement, TRUE);
	}
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonOrderAdd() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CComboBox& ccomboBoxTable = pParent->m_PageSelect2.m_cComboTable;

	LV_FINDINFO lvfindinfo;
	memset (&lvfindinfo, 0, sizeof (LV_FINDINFO));
	lvfindinfo.flags = LVFI_STRING;
	int nIdx = -1, nSel = m_cListBoxOrderBy.GetCurSel();
	if (nSel == LB_ERR)
		return;

	try
	{
		CString strItem;
		m_cListBoxOrderBy.GetText (nSel, strItem);
		//
		// Check if the column has already been added:
		lvfindinfo.psz = (LPCTSTR)strItem;
		if (m_cListCtrl.FindItem (&lvfindinfo) != -1)
			return;

		CaSqlWizardDataField* pObject = NULL;
		pObject = (CaSqlWizardDataField*) m_cListBoxOrderBy.GetItemData(nSel);

		if (!pObject)
			return;

		CaSqlWizardDataFieldOrder* pData = new CaSqlWizardDataFieldOrder (pObject->m_iType, pObject->m_strColumn, m_cListCtrl.GetDefaultSort());
		pData->SetTable(pObject->GetTable(), pObject->GetTableOwner(), pObject->GetObjectType());
		pData->m_bMultiTablesSelected = (ccomboBoxTable.GetCount() > 1) ? TRUE : FALSE;

		int nCount = m_cListCtrl.GetItemCount();
		nSel = m_cListCtrl.InsertItem (nCount, _T(""));
		if (nSel != -1)
		{
			m_cListCtrl.SetItemData  (nSel, (LPARAM)pData);
			m_cListCtrl.UpdateDisplay(nSel);
		}
		else
			delete pData;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return;
	}
	catch(...)
	{
		AfxMessageBox (_T("Internal error: \nCuDlgPropertyPageSqlWizardSelect3::OnButtonOrderAdd()."), MB_ICONEXCLAMATION|MB_OK);
		return;
	}

	EnableButtons();
}


void CuDlgPropertyPageSqlWizardSelect3::OnButtonOrderRemove() 
{
	int nSel = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
	if (nSel != -1)
		m_cListCtrl.DeleteItem (nSel);
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonOrderUp() 
{
	int nSelected = -1, nCount = m_cListCtrl.GetItemCount();
	nSelected = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);

	if (nSelected == -1 || nSelected == 0 || nCount == 1)
		return;
	//
	// Current Item
	CaSqlWizardDataFieldOrder* pDataCur = (CaSqlWizardDataFieldOrder*)m_cListCtrl.GetItemData (nSelected);
	//
	// Up Item
	CaSqlWizardDataFieldOrder* pDataUp  = (CaSqlWizardDataFieldOrder*)m_cListCtrl.GetItemData (nSelected-1);

	m_cListCtrl.SetItemData (nSelected,   (LPARAM)pDataUp);
	m_cListCtrl.SetItemData (nSelected-1, (LPARAM)pDataCur);
	m_cListCtrl.SetItemState(nSelected-1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_cListCtrl.UpdateDisplay();
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnButtonOrderDown() 
{
	int nSelected = -1, nCount = m_cListCtrl.GetItemCount();
	nSelected = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
	if (nSelected != -1 && nSelected < (nCount-1))
	{
		//
		// Current Item
		CaSqlWizardDataFieldOrder* pDataCur = (CaSqlWizardDataFieldOrder*)m_cListCtrl.GetItemData (nSelected);
		//
		// Down Item
		CaSqlWizardDataFieldOrder* pDataDown  = (CaSqlWizardDataFieldOrder*)m_cListCtrl.GetItemData (nSelected+1);
		
		m_cListCtrl.SetItemData (nSelected,   (LPARAM)pDataDown);
		m_cListCtrl.SetItemData (nSelected+1, (LPARAM)pDataCur);
		m_cListCtrl.SetItemState(nSelected+1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		m_cListCtrl.UpdateDisplay();
	}
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnCheckGroupBy() 
{
	int nShow = (m_cCheckGroupBy.GetCheck() == 1)? SW_SHOW: SW_HIDE;

	m_cButtonGUp.ShowWindow (nShow);
	m_cButtonGDown.ShowWindow (nShow);
	m_cListBoxGroupBy.ShowWindow (nShow);
	m_cStaticHaving.ShowWindow (nShow);
	m_cEditHaving.ShowWindow (nShow);
	m_cButtonHaving.ShowWindow (nShow);
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnExpressionAssistant(TCHAR tchszWH)
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CxDlgPropertySheetSqlExpressionWizard wizdlg;
	wizdlg.m_queryInfo = pParent->m_queryInfo;
	wizdlg.m_nFamilyID    = FF_PREDICATES;
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
		CComboBox& comboTable = pParent->m_PageSelect2.m_cComboTable;
		int nSel = -1;
		int i, nCount = comboTable.GetCount();
		for (i=0; i<nCount; i++)
		{
			pObject = (CaDBObject*)comboTable.GetItemData (i);
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
		if (tchszWH == _T('W'))
			m_cEditWhere.ReplaceSel (strStatement);
		else
		if (tchszWH == _T('H'))
			m_cEditHaving.ReplaceSel (strStatement);
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

void CuDlgPropertyPageSqlWizardSelect3::OnSelchangeListGroupBy() 
{
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnSelchangeListColumn() 
{
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardSelect3::OnItemchangedListOrderColumn(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	EnableButtons();
	*pResult = 0;
}

void CuDlgPropertyPageSqlWizardSelect3::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

void CuDlgPropertyPageSqlWizardSelect3::CleanListBoxGroupBy()
{
	int i,nCount = m_cListBoxGroupBy.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaSqlWizardDataField* pObject = (CaSqlWizardDataField*)m_cListBoxGroupBy.GetItemData (i);
		if (pObject)
			delete pObject;
		m_cListBoxGroupBy.SetItemData (i, (DWORD)0);
	}
}

void CuDlgPropertyPageSqlWizardSelect3::CleanListBoxOrderBy()
{
	int i,nCount = m_cListBoxOrderBy.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaSqlWizardDataField* pObject = (CaSqlWizardDataField*)m_cListBoxOrderBy.GetItemData (i);
		if (pObject)
			delete pObject;
		m_cListBoxOrderBy.SetItemData (i, (DWORD)0);
	}
	m_cListBoxOrderBy.ResetContent();
}

BOOL CuDlgPropertyPageSqlWizardSelect3::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_SEL3);
}

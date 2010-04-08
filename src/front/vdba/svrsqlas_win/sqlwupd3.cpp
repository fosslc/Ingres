/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwupd3.cpp, Implementation File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 3)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 01-Mars-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqlwpsht.h"
#include "sqlwupd3.h"
#include "dmlcolum.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardUpdate3, CPropertyPage)

CuDlgPropertyPageSqlWizardUpdate3::CuDlgPropertyPageSqlWizardUpdate3() : CPropertyPage(CuDlgPropertyPageSqlWizardUpdate3::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardUpdate3)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_SQL_UPDATE3);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardUpdate3::~CuDlgPropertyPageSqlWizardUpdate3()
{
}

void CuDlgPropertyPageSqlWizardUpdate3::EnableButtons()
{
	BOOL bEnable = FALSE;
	int nCount = m_cListCtrl.GetItemCount();
	if (nCount > 0 && m_cListCtrl.GetSelectedCount() == 1)
	{
		int nSel = m_cComboValues.GetCurSel();
		if (nSel != CB_ERR)
			bEnable = TRUE;
	}
	m_cButtonChoose.EnableWindow (bEnable);
}

void CuDlgPropertyPageSqlWizardUpdate3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardUpdate3)
	DDX_Control(pDX, IDC_IMAGE_FINISH, m_bitmap);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonChoose);
	DDX_Control(pDX, IDC_COMBO1, m_cComboValues);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardUpdate3, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardUpdate3)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonChooseValue)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboValues)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardUpdate3 message handlers

BOOL CuDlgPropertyPageSqlWizardUpdate3::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnInitDialog();
	m_cListCtrl.m_queryInfo = pParent->m_queryInfo;
	int nSel = CB_ERR;
	CComboBox& comboTable = pParent->m_PageUpdate1.m_cComboTable;
	nSel = comboTable.GetCurSel();
	if (nSel != CB_ERR)
	{
		CaDBObject* pObj = (CaDBObject*)comboTable.GetItemData (nSel);
		m_cListCtrl.m_queryInfo.SetItem2 (pObj->GetName(), pObj->GetOwner());
		m_cListCtrl.m_queryInfo.SetObjectType(pObj->GetObjectID());
	}

	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);

	CString strItem;
	LVCOLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[2] =
	{
		{IDS_HEADER_COLUMN,  100, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_VALUE,   300, LVCFMT_LEFT, FALSE}
	};
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
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
	
	m_cListCtrl.Invalidate();
	EnableButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlWizardUpdate3::OnDestroy() 
{
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardUpdate3::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

BOOL CuDlgPropertyPageSqlWizardUpdate3::OnSetActive() 
{
	CWaitCursor doWaitCursor;
	int i, idx, nCount;
	CString strItem;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);

	CStringList& listStrColumn   = pParent->m_PageUpdate2.m_listStrColumn;
	CuCheckListBox& checkListBox = pParent->m_PageUpdate1.m_cCheckListBoxColumn;
	m_cComboValues.ResetContent();
	POSITION pos = listStrColumn.GetHeadPosition();
	while (pos != NULL)
	{
		strItem = listStrColumn.GetNext (pos);
		m_cComboValues.AddString (strItem);
	}
	m_cComboValues.SetCurSel(0);
	//
	// Remove the columns in 'm_cListCtrl' that are not (checked) in 'checkListBox':
	nCount = m_cListCtrl.GetItemCount();
	i = 0;
	while (i < nCount && nCount > 0)
	{
		strItem = m_cListCtrl.GetItemText (i, 0);
		idx = checkListBox.FindStringExact (-1, strItem);
		if (idx == LB_ERR || !checkListBox.GetCheck(idx))
		{
			m_cListCtrl.DeleteItem (i);
			nCount = m_cListCtrl.GetItemCount();
			continue;
		}
		i++;
	}

	CaColumn* lpData = NULL;
	//
	// Add the columns that are checked in 'checkListBox' but not in 'm_cListCtrl':
	LVFINDINFO lvfindinfo;
	idx = -1;
	memset (&lvfindinfo, 0, sizeof (LVFINDINFO));
	lvfindinfo.flags    = LVFI_STRING;
	nCount = checkListBox.GetCount();
	for (i=0; i<nCount; i++)
	{
		lpData = (CaColumn*)checkListBox.GetItemData(i);
		if (checkListBox.GetCheck(i) && lpData)
		{
			checkListBox.GetText (i, strItem);
			lvfindinfo.psz = (LPCTSTR)strItem;
			if (m_cListCtrl.FindItem (&lvfindinfo) == -1)
			{
				idx = m_cListCtrl.GetItemCount();
				m_cListCtrl.InsertItem  (idx, strItem);
				m_cListCtrl.SetItemData (idx, (LPARAM)INGRESII_llIngresColumnType2AppType(lpData));
			}
		}
	}
	EnableButtons();

	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlWizardUpdate3::OnWizardFinish() 
{
	CWaitCursor doWaitCursor;
	int i, idx, nCount1, nCount2;
	CString strStatement; 
	CString strItem;
	CString strValue;

	m_cListCtrl.HideProperty (TRUE);
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CComboBox& comboTable      = pParent->m_PageUpdate1.m_cComboTable;
	CuListCtrlCheckBox& listTable  = pParent->m_PageUpdate2.m_cListTable;
	CuListCtrlCheckBox& listView   = pParent->m_PageUpdate2.m_cListView;
	CEdit& editCriteria        = pParent->m_PageUpdate2.m_cEditCriteria;
	idx = comboTable.GetCurSel();
	if (idx == CB_ERR)
	{
		pParent->SetStatement (NULL);
		return TRUE;
	}
	//
	// Check Values:
	nCount1 = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount1; i++)
	{
		strValue = m_cListCtrl.GetItemText (i, CuEditableListCtrlSqlWizardUpdateValue::VALUE);
		strValue.TrimLeft();
		strValue.TrimRight();
		if (strValue.IsEmpty())
		{
			// _T("Not all the columns have values.\nDo you want to continue ?"); //UK IDS_A_COLUMNS_VALUE
			if (AfxMessageBox (IDS_MSG_COLUMNS_VALUE, MB_ICONEXCLAMATION|MB_OKCANCEL) == IDOK)
				break;
			else
			{
				return FALSE;
			}
		}
	}

	//
	// Generate: UPDATE x
	strStatement.Format (_T(" %s "), cstrUpdate);
	CaDBObject* pTable = (CaDBObject*)comboTable.GetItemData(idx);
	ASSERT(pTable);
	if(!pTable)
		return FALSE;
	if (pTable->GetOwner().IsEmpty())
		strStatement += INGRESII_llQuoteIfNeeded(pTable->GetName());
	else
	{
		strStatement += INGRESII_llQuoteIfNeeded(pTable->GetOwner());
		strStatement += _T(".");
		strStatement += INGRESII_llQuoteIfNeeded(pTable->GetName());
	}

	//
	// Generate: FROM a, b, c:
	BOOL bOne = TRUE;
	CString strListItem = _T("");
	nCount1 = listTable.GetItemCount();
	nCount2 = listView.GetItemCount();
	for (i=0; i<nCount1; i++)
	{
		if (listTable.GetItemChecked(i))
		{
			CaDBObject* pTable =(CaDBObject*)listTable.GetItemData(i);
			ASSERT(pTable);
			if (!pTable)
				continue;
			if (!bOne)
				strListItem += _T(", ");
			bOne = FALSE;
			if (pTable->GetOwner().IsEmpty())
				strListItem += INGRESII_llQuoteIfNeeded(pTable->GetName());
			else
			{
				strListItem += INGRESII_llQuoteIfNeeded(pTable->GetOwner());
				strListItem += _T(".");
				strListItem += INGRESII_llQuoteIfNeeded(pTable->GetName());
			}
		}
	}
	for (i=0; i<nCount2; i++)
	{
		if (listView.GetItemChecked(i))
		{
			CaDBObject* pView =(CaDBObject*)listView.GetItemData(i);
			ASSERT(pView);
			if (!pView)
				continue;
			if (!bOne)
				strListItem += _T(", ");
			bOne = FALSE;
			if (pView->GetOwner().IsEmpty())
				strListItem += INGRESII_llQuoteIfNeeded(pView->GetName());
			else
			{
				strListItem += INGRESII_llQuoteIfNeeded(pView->GetOwner());
				strListItem += _T(".");
				strListItem += INGRESII_llQuoteIfNeeded(pView->GetName());
			}

		}
	}
	if (!strListItem.IsEmpty())
	{
		strStatement += _T(" ");
		strStatement += cstrFrom;
		strStatement += _T(" ");
		strStatement += strListItem;
	}
	//
	// SET column = expr ...:
	strListItem = _T("");
	bOne = TRUE;
	nCount1 = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount1; i++)
	{
		strItem  = m_cListCtrl.GetItemText (i, CuEditableListCtrlSqlWizardUpdateValue::COLUMN);
		strValue = m_cListCtrl.GetItemText (i, CuEditableListCtrlSqlWizardUpdateValue::VALUE);
		strValue.TrimLeft();
		strValue.TrimRight();
		if (strValue.IsEmpty())
			continue;
		if (!bOne)
			strListItem += _T(", ");
		bOne = FALSE;
		if (strListItem.IsEmpty())
		{
			strListItem += cstrSet;
			strListItem += _T(" ");
		}

		strListItem += INGRESII_llQuoteIfNeeded(strItem);
		strListItem += _T(" = ");
		strListItem += strValue;
	}
	if (strListItem.IsEmpty())
	{
		pParent->SetStatement (NULL);
		return TRUE;
	}
	strStatement += _T(" ");
	strStatement += strListItem;

	//
	// Generate the WHERE ...:
	editCriteria.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	if (!strItem.IsEmpty())
	{
		strStatement += _T(" ");
		strStatement += cstrWhere;
		strStatement += _T(" ");
		strStatement += strItem;
	}
	pParent->SetStatement (strStatement);

	return CPropertyPage::OnWizardFinish();
}

void CuDlgPropertyPageSqlWizardUpdate3::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	EnableButtons();
	*pResult = 0;
}

void CuDlgPropertyPageSqlWizardUpdate3::OnButtonChooseValue()
{
	CWaitCursor doWaitCursor;
	int index = -1, nSel = m_cComboValues.GetCurSel();
	if (nSel == CB_ERR)
		return;
	CString strValue;
	m_cComboValues.GetLBText (nSel, strValue);

	index = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
	if (index == -1)
		return;
	CRect rect, rCell;
	m_cListCtrl.GetItemRect (index, rect, LVIR_BOUNDS);
	if (!m_cListCtrl.GetCellRect (rect, CuEditableListCtrlSqlWizardUpdateValue::VALUE, rCell))
		return;
	rCell.top    -= 2;
	rCell.bottom += 2;
	m_cListCtrl.HideProperty();
	m_cListCtrl.SetEditText (index, CuEditableListCtrlSqlWizardUpdateValue::VALUE, rCell, strValue, FALSE);
}

void CuDlgPropertyPageSqlWizardUpdate3::OnSelchangeComboValues() 
{
	EnableButtons();
}

void CuDlgPropertyPageSqlWizardUpdate3::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardUpdate3::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_UPD3);
}

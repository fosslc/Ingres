/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlwins2.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Insert Page 2)
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
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "sqlwpsht.h"
#include "sqlwins2.h"
#include "dmlcolum.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardInsert2, CPropertyPage)

CuDlgPropertyPageSqlWizardInsert2::CuDlgPropertyPageSqlWizardInsert2() : CPropertyPage(CuDlgPropertyPageSqlWizardInsert2::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardInsert2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_SQL_INSERT2);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardInsert2::~CuDlgPropertyPageSqlWizardInsert2()
{
}

void CuDlgPropertyPageSqlWizardInsert2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardInsert2)
	DDX_Control(pDX, IDC_IMAGE_FINISH, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardInsert2, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardInsert2)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardInsert2 message handlers

BOOL CuDlgPropertyPageSqlWizardInsert2::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CPropertyPage::OnInitDialog();
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
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
	if (m_strAll.IsEmpty())
	{
		if (m_strAll.LoadString (IDS_SELECTALL) == 0)
			m_strAll = _T("<All Columns>");
	}
	m_cListCtrl.m_queryInfo = pParent->m_queryInfo;

	int nSel = CB_ERR;
	CComboBox& comboTable = pParent->m_PageInsert1.m_cComboTable;
	nSel = comboTable.GetCurSel();
	if (nSel != CB_ERR)
	{
		CaDBObject* pObj = (CaDBObject*)comboTable.GetItemData(nSel);
		m_cListCtrl.m_queryInfo.SetItem2 (pObj->GetName(), pObj->GetOwner());
		if (pParent->m_PageInsert1.IsDlgButtonChecked (IDC_RADIO1))
			m_cListCtrl.m_queryInfo.SetObjectType(OBT_TABLE);
		else
			m_cListCtrl.m_queryInfo.SetObjectType(OBT_VIEW);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlWizardInsert2::OnDestroy() 
{
	m_ImageList.DeleteImageList();
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardInsert2::OnSetActive() 
{
	CWaitCursor doWaitCursor;

	int i, nCol, nCount;
	BOOL bExit = FALSE;
	LVFINDINFO lvfindinfo;
	memset (&lvfindinfo, 0, sizeof (LVFINDINFO));
	lvfindinfo.flags = LVFI_STRING;

	CString strItem;
	CaColumn* lpData = NULL;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	CuCheckListBox& checkListBox = pParent->m_PageInsert1.m_cCheckListBoxColumn;

	//
	// Remove the columns out of 'm_cListCtrl' if they are not choosed in Page 1:
	nCount = m_cListCtrl.GetItemCount();
	i = 0;
	while (i < nCount && nCount > 0)
	{
		strItem = m_cListCtrl.GetItemText(i, CuEditableListCtrlSqlWizardInsertValue::COLUMN);
		nCol = checkListBox.FindStringExact (-1, strItem);
		if (nCol == LB_ERR || !checkListBox.GetCheck(nCol))
		{
			m_cListCtrl.DeleteItem (i);
			nCount = m_cListCtrl.GetItemCount();
			continue;
		}
		i++;
	}

	//
	// Add the columns checked in page 1 if they are not in 'm_cListCtrl':
	nCount = checkListBox.GetCount();
	for (i=0; i<nCount; i++)
	{
		lpData = (CaColumn*)checkListBox.GetItemData(i);
		if (checkListBox.GetCheck(i) && lpData)
		{
			checkListBox.GetText (i, strItem);
			nCol = m_cListCtrl.GetItemCount();
			lvfindinfo.psz = (LPCTSTR)strItem;
			if (m_cListCtrl.FindItem (&lvfindinfo) == -1)
			{
				m_cListCtrl.InsertItem  (nCol, strItem);
				m_cListCtrl.SetItemData (nCol, (LPARAM)INGRESII_llIngresColumnType2AppType(lpData));
			}
		}
	}


	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlWizardInsert2::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

BOOL CuDlgPropertyPageSqlWizardInsert2::OnWizardFinish() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		BOOL bOne = TRUE;
		CString strItem;
		CString strValue;
		CString listStrItem   = _T("");
		CString listStrValues = _T("");
		CString strStatement;

		m_cListCtrl.HideProperty (TRUE);
		int i, nCount = m_cListCtrl.GetItemCount();
		//
		// Check Values:
		for (i=0; i<nCount; i++)
		{
			strValue = m_cListCtrl.GetItemText (i, CuEditableListCtrlSqlWizardInsertValue::VALUE);
			strValue.TrimLeft();
			strValue.TrimRight();
			if (strValue.IsEmpty())
			{
				//_T("Not all the columns have values.\nDo you want to continue ?");
				if (AfxMessageBox (IDS_MSG_COLUMNS_VALUE, MB_ICONEXCLAMATION|MB_OKCANCEL) == IDOK)
					break;
				else
				{
					return FALSE;
				}
			}
		}

		for (i=0; i<nCount; i++)
		{
			strItem  = m_cListCtrl.GetItemText (i, CuEditableListCtrlSqlWizardInsertValue::COLUMN);
			strValue = m_cListCtrl.GetItemText (i, CuEditableListCtrlSqlWizardInsertValue::VALUE);
			strValue.TrimLeft();
			strValue.TrimRight();
			if (!strValue.IsEmpty())
			{
				if (!bOne)
				{
					listStrItem   += _T(", ");
					listStrValues += _T(", ");
				}
				bOne = FALSE;
				listStrItem   += INGRESII_llQuoteIfNeeded(strItem);
				listStrValues += strValue;
			}
		}
		if (!listStrItem.IsEmpty())
		{
			CComboBox& comboTable = pParent->m_PageInsert1.m_cComboTable;
			i = comboTable.GetCurSel();
			if (i==CB_ERR)
			{
				pParent->SetStatement (NULL);
				return TRUE;
			}
			CaDBObject* pTable = (CaDBObject*)comboTable.GetItemData(i);
			CString csTmpName;
			if (!pTable->GetOwner().IsEmpty())
			{
				csTmpName = INGRESII_llQuoteIfNeeded(pTable->GetOwner());
				csTmpName += _T(".");
				csTmpName += INGRESII_llQuoteIfNeeded(pTable->GetName());
			}
			else
				csTmpName = INGRESII_llQuoteIfNeeded(pTable->GetName());

			strStatement.Format (
				_T("%s %s %s (%s) %s (%s)"),
				cstrInsert,
				cstrInto,
				(LPCTSTR)csTmpName,
				(LPCTSTR)listStrItem,
				cstrValues,
				(LPCTSTR)listStrValues);
			pParent->SetStatement (strStatement);
		}
		else
			pParent->SetStatement (NULL);
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

void CuDlgPropertyPageSqlWizardInsert2::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardInsert2::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_INS2);
}

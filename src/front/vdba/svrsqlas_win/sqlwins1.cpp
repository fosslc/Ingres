/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlwins1.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Insert Page 1)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
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
#include "sqlwins1.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardInsert1, CPropertyPage)

CuDlgPropertyPageSqlWizardInsert1::CuDlgPropertyPageSqlWizardInsert1() : CPropertyPage(CuDlgPropertyPageSqlWizardInsert1::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardInsert1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_strAll = _T("");
	m_bitmap.SetBitmpapId (IDB_SQL_INSERT1);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardInsert1::~CuDlgPropertyPageSqlWizardInsert1()
{
}

void CuDlgPropertyPageSqlWizardInsert1::EnableWizardButtons()
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CString strItem;
	BOOL bEnable = FALSE;
	BOOL bManual = IsDlgButtonChecked (IDC_RADIO3);
	int i, nCount = m_cCheckListBoxColumn.GetCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cCheckListBoxColumn.GetCheck(i))
		{
			m_cCheckListBoxColumn.GetText (i, strItem);
			if (strItem.CompareNoCase (m_strAll) != 0)
			{
				bEnable = TRUE;
				break;
			}
		}
	}
	if (bManual)
	{
		if (bEnable)
			pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
		else
			pParent->SetWizardButtons(PSWIZB_BACK);
	}
	else
		pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
}

void CuDlgPropertyPageSqlWizardInsert1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardInsert1)
	DDX_Control(pDX, IDC_IMAGE_INSERT, m_bitmap);
	DDX_Control(pDX, IDC_COMBO1, m_cComboTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardInsert1, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardInsert1)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioTable)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioView)
	ON_BN_CLICKED(IDC_RADIO3, OnRadioManual)
	ON_BN_CLICKED(IDC_RADIO4, OnRadioSubSelect)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboTable)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_CLBN_CHKCHANGE (IDC_LIST1, OnCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardInsert1 message handlers

BOOL CuDlgPropertyPageSqlWizardInsert1::OnSetActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	EnableWizardButtons();

	return CPropertyPage::OnSetActive();
}

void CuDlgPropertyPageSqlWizardInsert1::OnRadioTable() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		SQLW_ComboBoxTablesClean(&m_cComboTable);
		CaLLQueryInfo info(pParent->m_queryInfo);
		info.SetObjectType(OBT_TABLE);
		info.SetFetchObjects(CaLLQueryInfo::FETCH_USER);

		SQLW_ComboBoxFillTables (&m_cComboTable, &info);
		m_cComboTable.SetCurSel (0);
		OnSelchangeComboTable();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		//_T("Cannot query Tables");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_TABLE, MB_ICONEXCLAMATION|MB_OK);
	}
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardInsert1::OnRadioView() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		SQLW_ComboBoxTablesClean(&m_cComboTable);

		CaLLQueryInfo info(pParent->m_queryInfo);
		info.SetObjectType(OBT_VIEW);
		info.SetFetchObjects(CaLLQueryInfo::FETCH_USER);
		SQLW_ComboBoxFillTables (&m_cComboTable, &info);
		m_cComboTable.SetCurSel (0);
		OnSelchangeComboTable();
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
		//_T("Cannot query Views");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_VIEW, MB_ICONEXCLAMATION|MB_OK);
	}
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardInsert1::OnRadioManual() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetCategoryInsert2(TRUE);
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardInsert1::OnRadioSubSelect() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetCategoryInsert2(FALSE);
	EnableWizardButtons();
}

//
// Initialize the columns of Current Selected Object (Table or View):
void CuDlgPropertyPageSqlWizardInsert1::OnSelchangeComboTable() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	int nSel = m_cComboTable.GetCurSel();
	SQLW_CuCheckListBoxColumnsClean(&m_cCheckListBoxColumn);
	if (nSel == CB_ERR)
		return;
	try
	{
		CString strAll;
		if (m_strAll.IsEmpty())
		{
			if (m_strAll.LoadString (IDS_SELECTALL) == 0)
				m_strAll = _T("<All Columns>");
		}
		CaDBObject* pTable = (CaDBObject*)m_cComboTable.GetItemData(nSel);
		ASSERT(pTable);
		if (!pTable)
			return;
		CaLLQueryInfo info(pParent->m_queryInfo);
		if (pTable->GetObjectID() == OBT_TABLE)
			info.SetObjectType(OBT_TABLECOLUMN);
		else
			info.SetObjectType(OBT_VIEWCOLUMN);
		info.SetItem2(pTable->GetName(), pTable->GetOwner());

		m_cCheckListBoxColumn.AddString (m_strAll);
		SQLW_CuCheckListBoxFillColumns (&m_cCheckListBoxColumn, &info);
		
		//
		// Select the previous selected columns if Any:
		//
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
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardInsert1::OnCheckChange()
{
	CWaitCursor doWaitCursor;
	int   i, nCount= 0, index = m_cCheckListBoxColumn.GetCaretIndex();
	if (index == LB_ERR)
		return;
	nCount = m_cCheckListBoxColumn.GetCount();
	CString strText;
	m_cCheckListBoxColumn.GetText (index, strText);
	if (m_strAll.CompareNoCase (strText) == 0)
	{
		if (m_cCheckListBoxColumn.GetCheck(index))
		{
			for (i=0; i<nCount; i++)
			{
				if (!m_cCheckListBoxColumn.GetCheck(i))
					m_cCheckListBoxColumn.SetCheck(i, TRUE);
			}
		}
		else
		{
			for (i=0; i<nCount; i++)
			{
				if (m_cCheckListBoxColumn.GetCheck(i))
					m_cCheckListBoxColumn.SetCheck(i, FALSE);
			}
		}
	}

	EnableWizardButtons();
}


BOOL CuDlgPropertyPageSqlWizardInsert1::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CPropertyPage::OnInitDialog();
	VERIFY (m_cCheckListBoxColumn.SubclassDlgItem (IDC_LIST1, this));
#if defined (_CHECKLISTBOX_USE_DEFAULT_GUI_FONT)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	m_cCheckListBoxColumn.SendMessage(WM_SETFONT, (WPARAM)hFont);
#endif

	// Check Table Radio Button:
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	//
	// Check Manual Radio Button:
	CheckRadioButton (IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
	OnRadioManual();

	//
	// Simulate the Click on Radio Table:
	OnRadioTable();
	EnableWizardButtons();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CuDlgPropertyPageSqlWizardInsert1::OnDestroy() 
{
	SQLW_ComboBoxTablesClean(&m_cComboTable);
	SQLW_CuCheckListBoxColumnsClean(&m_cCheckListBoxColumn);

	CPropertyPage::OnDestroy();
}


BOOL CuDlgPropertyPageSqlWizardInsert1::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlWizardInsert1::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardInsert1::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_INS1);
}

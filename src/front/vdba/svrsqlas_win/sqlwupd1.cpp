/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlwupd1.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 1)
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
#include "sqlwupd1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardUpdate1, CPropertyPage)

CuDlgPropertyPageSqlWizardUpdate1::CuDlgPropertyPageSqlWizardUpdate1() : CPropertyPage(CuDlgPropertyPageSqlWizardUpdate1::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardUpdate1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_strAll = _T("");
	m_bitmap.SetBitmpapId (IDB_SQL_UPDATE1);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardUpdate1::~CuDlgPropertyPageSqlWizardUpdate1()
{
}

void CuDlgPropertyPageSqlWizardUpdate1::EnableWizardButtons()
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CString strItem;
	BOOL bEnable = FALSE;
	int i, nCount = m_cCheckListBoxColumn.GetCount();
	//
	// Skip the first item:
	for (i=1; i<nCount; i++)
	{
		if (m_cCheckListBoxColumn.GetCheck(i))
		{
			bEnable = TRUE;
			break;
		}
	}
	if (bEnable)
		pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_BACK);
}


void CuDlgPropertyPageSqlWizardUpdate1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardUpdate1)
	DDX_Control(pDX, IDC_IMAGE_UPDATE, m_bitmap);
	DDX_Control(pDX, IDC_COMBO1, m_cComboTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardUpdate1, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardUpdate1)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioTable)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioView)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboTable)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_CLBN_CHKCHANGE (IDC_LIST1, OnCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardUpdate1 message handlers

BOOL CuDlgPropertyPageSqlWizardUpdate1::OnSetActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	EnableWizardButtons();
	return CPropertyPage::OnSetActive();
}

void CuDlgPropertyPageSqlWizardUpdate1::OnRadioTable() 
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
		// _T("Cannot Query Tables");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_TABLE, MB_ICONEXCLAMATION|MB_OK);
	}
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardUpdate1::OnRadioView() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		SQLW_ComboBoxTablesClean(&m_cComboTable);
		CaLLQueryInfo info(pParent->m_queryInfo);
		info.SetObjectType(OBT_VIEW);

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
		// _T("Cannot query Views"); 
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_VIEW, MB_ICONEXCLAMATION|MB_OK);
	}
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardUpdate1::OnDestroy()
{
	SQLW_ComboBoxTablesClean(&m_cComboTable);
	SQLW_CuCheckListBoxColumnsClean(&m_cCheckListBoxColumn);
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardUpdate1::OnInitDialog() 
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
	if (m_strAll.IsEmpty())
	{
		if (m_strAll.LoadString (IDS_SELECTALL) == 0)
			m_strAll = _T("<All Columns>");
	}

	// Check Table Radio Button:
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	//
	// Simulate the Click on Radio Table:
	OnRadioTable();
	EnableWizardButtons();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuDlgPropertyPageSqlWizardUpdate1::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlWizardUpdate1::OnSelchangeComboTable() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CString strMsg;
	int nSel = m_cComboTable.GetCurSel();

	SQLW_CuCheckListBoxColumnsClean(&m_cCheckListBoxColumn);
	if (nSel == CB_ERR)
		return;
	try
	{
		CaDBObject* pTable = (CaDBObject*)m_cComboTable.GetItemData(nSel);
		ASSERT(pTable);
		if (!pTable)
			return;
		if (m_strAll.IsEmpty())
		{
			if (m_strAll.LoadString (IDS_SELECTALL) == 0)
				m_strAll = _T("<All Columns>");
		}
		m_cCheckListBoxColumn.AddString (m_strAll);
		CaLLQueryInfo info(pParent->m_queryInfo);
		if (pTable->GetObjectID() == OBT_TABLE)
			info.SetObjectType(OBT_TABLECOLUMN);
		else
			info.SetObjectType(OBT_VIEWCOLUMN);
		info.SetItem2(pTable->GetName(), pTable->GetOwner());

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

void CuDlgPropertyPageSqlWizardUpdate1::OnCheckChange()
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CString strItem;
	BOOL bEnable = FALSE;

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

void CuDlgPropertyPageSqlWizardUpdate1::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardUpdate1::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_UPD1);
}

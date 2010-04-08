/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwdel1.cpp, Implementation File  
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Delete Page 1)
**
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
#include "sqlepsht.h"
#include "sqlwdel1.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardDelete1, CPropertyPage)

CuDlgPropertyPageSqlWizardDelete1::CuDlgPropertyPageSqlWizardDelete1() : CPropertyPage(CuDlgPropertyPageSqlWizardDelete1::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardDelete1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_SQL_DELETE1);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardDelete1::~CuDlgPropertyPageSqlWizardDelete1()
{
}

void CuDlgPropertyPageSqlWizardDelete1::EnableWizardButtons()
{
	int nCount = m_cComboTable.GetCount();
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	if (nCount == 0)
		pParent->SetWizardButtons(PSWIZB_DISABLEDFINISH|PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlWizardDelete1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardDelete1)
	DDX_Control(pDX, IDC_IMAGE_DELETE, m_bitmap);
	DDX_Control(pDX, IDC_EDIT1, m_cEditSearchCondition);
	DDX_Control(pDX, IDC_COMBO1, m_cComboTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardDelete1, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardDelete1)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioTable)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioView)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonCriteria)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardDelete1 message handlers

BOOL CuDlgPropertyPageSqlWizardDelete1::OnSetActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	return CPropertyPage::OnSetActive();
}

void CuDlgPropertyPageSqlWizardDelete1::OnRadioTable() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		SQLW_ComboBoxTablesClean(&m_cComboTable);
		
		CaLLQueryInfo info(pParent->m_queryInfo);
		info.SetObjectType(OBT_TABLE);
		SQLW_ComboBoxFillTables (&m_cComboTable, &info);
		m_cComboTable.SetCurSel (0);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		// _T("Cannot query Tables"); 
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_TABLE, MB_ICONEXCLAMATION|MB_OK);
	}
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlWizardDelete1::OnRadioView() 
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

BOOL CuDlgPropertyPageSqlWizardDelete1::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlWizardDelete1::OnDestroy() 
{
	SQLW_ComboBoxTablesClean(&m_cComboTable);
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardDelete1::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CPropertyPage::OnInitDialog();
	// Check Table Radio Button:
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	//
	// Simulate the Click on Radio Table:
	OnRadioTable();
	EnableWizardButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuDlgPropertyPageSqlWizardDelete1::OnWizardFinish() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		CString strTable;
		CString strText;
		CString strStatement;
		CString strNameQuoted;

		strStatement.Format (_T("%s %s "), cstrDelete, cstrFrom);
		int nSel = m_cComboTable.GetCurSel();
		if (nSel != CB_ERR)
		{
			CaDBObject* pObj = (CaDBObject*)m_cComboTable.GetItemData(nSel);
			ASSERT(pObj);
			if (!pObj)
				return FALSE;

			if (pObj->GetOwner().IsEmpty())
				strNameQuoted  = INGRESII_llQuoteIfNeeded(pObj->GetName());
			else
			{
				strNameQuoted  = INGRESII_llQuoteIfNeeded(pObj->GetOwner());
				strNameQuoted += _T(".");
				strNameQuoted += INGRESII_llQuoteIfNeeded(pObj->GetName());
			}

			strStatement += strNameQuoted;
			m_cEditSearchCondition.GetWindowText (strText);
			strText.TrimLeft();
			strText.TrimRight();
			if (!strText.IsEmpty())
			{
				strStatement += _T(" ");
				strStatement += cstrWhere;
				strStatement += _T(" ");
				strStatement += strText;
			}
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

void CuDlgPropertyPageSqlWizardDelete1::OnButtonCriteria() 
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
		int nSel = -1;
		//
		// Add the current Table:
		nSel = m_cComboTable.GetCurSel();
		if (nSel != CB_ERR)
		{
			CaDBObject* pObj = (CaDBObject*)m_cComboTable.GetItemData (nSel);
			CaDBObject* pNewObject = new CaDBObject(*pObj);
			if (IsDlgButtonChecked (IDC_RADIO1))
				pNewObject->SetObjectID(OBT_TABLE);
			else
				pNewObject->SetObjectID(OBT_VIEW);
			wizdlg.m_listObject.AddTail (pNewObject);
		}
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
	int nResult = wizdlg.DoModal();
	if (nResult == IDCANCEL)
		return;

	CString strStatement;
	wizdlg.GetStatement (strStatement);
	if (strStatement.IsEmpty())
		return;
	m_cEditSearchCondition.ReplaceSel (strStatement);
}

void CuDlgPropertyPageSqlWizardDelete1::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardDelete1::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_DEL1);
}

/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqleepar.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Any-All-Exist Paramter page)
**
** History:
**
** xx-Jun-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqleepar.h"
#include "sqlepsht.h"
#include "sqlwpsht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionAnyAllExistParams, CPropertyPage)

CuDlgPropertyPageSqlExpressionAnyAllExistParams::CuDlgPropertyPageSqlExpressionAnyAllExistParams() : CPropertyPage(CuDlgPropertyPageSqlExpressionAnyAllExistParams::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionAnyAllExistParams)
	m_strParam1 = _T("");
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	m_strHelp3 = _T("");
	m_strParam1Name = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionAnyAllExistParams::~CuDlgPropertyPageSqlExpressionAnyAllExistParams()
{
}




void CuDlgPropertyPageSqlExpressionAnyAllExistParams::UpdateDisplay (LPGENFUNCPARAMS lpFparam)
{
	//
	// Function Name:
	if (lpFparam->FuncName[0])
		m_strFunctionName = lpFparam->FuncName;
	else
		m_strFunctionName = _T("");
	//
	// Help Text:
	if (lpFparam->HelpText1[0])
		m_strHelp1= lpFparam->HelpText1;
	else
		m_strHelp1= _T("");
	if (lpFparam->HelpText2[0])
		m_strHelp2= lpFparam->HelpText2;
	else
		m_strHelp2= _T("");
	if (lpFparam->HelpText3[0])
		m_strHelp3= lpFparam->HelpText3;
	else
		m_strHelp3 = _T("");
	//
	// Parameter caption:
	if (lpFparam->CaptionText1[0])
		m_strParam1Name.Format (_T("%s:"), lpFparam->CaptionText1);
	else
		m_strParam1Name =  _T("");

	m_strParam1 = _T("");
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	UpdateData (FALSE);
	m_cParam1.SetFocus();
}

void CuDlgPropertyPageSqlExpressionAnyAllExistParams::EnableWizardButtons()
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CString m_strText1;
	
	m_cParam1.GetWindowText (m_strText1);
	m_strText1.TrimLeft();
	m_strText1.TrimRight();

	if (m_strText1.IsEmpty())
		pParent->SetWizardButtons(PSWIZB_DISABLEDFINISH|PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlExpressionAnyAllExistParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionAnyAllExistParams)
	DDX_Control(pDX, IDC_EDIT1, m_cParam1);
	DDX_Text(pDX, IDC_EDIT1, m_strParam1);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	DDX_Text(pDX, IDC_STATIC4, m_strHelp3);
	DDX_Text(pDX, IDC_STATIC5, m_strParam1Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionAnyAllExistParams, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionAnyAllExistParams)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1Param)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionAnyAllExistParams message handlers

BOOL CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnSetActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	GENFUNCPARAMS fparam;
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
	{
		memset (&fparam, 0, sizeof(fparam));
		FillFunctionParameters (&fparam, lpComp);
		UpdateDisplay (&fparam);
	}
	EnableWizardButtons();
	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnWizardFinish() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	GENFUNCPARAMS fparam;
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
	{
		memset (&fparam, 0, sizeof(fparam));
		FillFunctionParameters (&fparam, lpComp);
		UpdateData (TRUE);
		//
		// Any, All, Exist function alway has 1 argument:
		CString strStatement;
		strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)m_strParam1);
		pParent->SetStatement (strStatement);
	}
	else
		pParent->SetStatement (NULL);
	return TRUE;
}

void CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnButton1Param() 
{
	int nAnswer = IDCANCEL;
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();

	try
	{
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
				m_cParam1.ReplaceSel(strStatement, TRUE);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch(...)
	{
		// _T("Cannot initialize the SQL assistant.");
		AfxMessageBox (IDS_MSG_SQL_ASSISTANT, MB_ICONEXCLAMATION|MB_OK);
	}
}

void CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnChangeEdit1() 
{
	EnableWizardButtons();
}

BOOL CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlExpressionAnyAllExistParams::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
		return APP_HelpInfo(m_hWnd, lpComp->GetID());
	return FALSE;
}

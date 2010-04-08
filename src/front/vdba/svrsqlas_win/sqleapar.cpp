/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : qleapar.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Aggregate Paramter page)
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
#include "sqleapar.h"
#include "sqlepsht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionAggregateParams, CPropertyPage)

CuDlgPropertyPageSqlExpressionAggregateParams::CuDlgPropertyPageSqlExpressionAggregateParams() : CPropertyPage(CuDlgPropertyPageSqlExpressionAggregateParams::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionAggregateParams)
	m_strParam1 = _T("");
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	m_strHelp3 = _T("");
	m_strParam1Name = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionAggregateParams::~CuDlgPropertyPageSqlExpressionAggregateParams()
{
}


void CuDlgPropertyPageSqlExpressionAggregateParams::UpdateDisplay (LPGENFUNCPARAMS lpFparam)
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
	if (lpFparam->CaptionText2[0])
		m_strParam1Name.Format (_T("%s:"), lpFparam->CaptionText2);
	else
		m_strParam1Name =  _T("");
	m_strParam1 = _T("");
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	UpdateData (FALSE);
}

void CuDlgPropertyPageSqlExpressionAggregateParams::EnableWizardButtons()
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


void CuDlgPropertyPageSqlExpressionAggregateParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionAggregateParams)
	DDX_Control(pDX, IDC_EDIT1, m_cParam1);
	DDX_Text(pDX, IDC_EDIT1, m_strParam1);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	DDX_Text(pDX, IDC_STATIC4, m_strHelp3);
	DDX_Text(pDX, IDC_STATIC5, m_strParam1Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionAggregateParams, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionAggregateParams)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1Param)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionAggregateParams message handlers

BOOL CuDlgPropertyPageSqlExpressionAggregateParams::OnSetActive() 
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

BOOL CuDlgPropertyPageSqlExpressionAggregateParams::OnWizardFinish() 
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
		// Aggregate functions alway have 2 arguments:
		CString strStatement;
		BOOL bAll = IsDlgButtonChecked (IDC_RADIO1);
		strStatement.Format ((LPCTSTR)fparam.resultformat2, (LPCTSTR)GetConstantAllDistinct(bAll), (LPCTSTR)m_strParam1);
		pParent->SetStatement (strStatement);
	}
	else
		pParent->SetStatement (NULL);
	return TRUE;
}



void CuDlgPropertyPageSqlExpressionAggregateParams::OnButton1Param() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CxDlgPropertySheetSqlExpressionWizard wizdlg;
	wizdlg.m_queryInfo = pParent->m_queryInfo;

	wizdlg.m_nFamilyID    = pParent->m_nFamilyID;
	wizdlg.m_nAggType     = pParent->m_nAggType;
	wizdlg.m_nCompare     = pParent->m_nCompare;
	wizdlg.m_nIn_notIn    = pParent->m_nIn_notIn;
	wizdlg.m_nSub_List    = pParent->m_nSub_List;
	//
	// Initialize Tables or Views:
	POSITION pos = pParent->m_listObject.GetHeadPosition();
	try
	{
		while (pos != NULL)
		{
			CaDBObject* pObject = pParent->m_listObject.GetNext (pos);
			CaDBObject* pNewObject = new CaDBObject(*pObject);
			pNewObject->SetObjectID(pObject->GetObjectID());
			wizdlg.m_listObject.AddTail (pNewObject);
		}
		
		pos = pParent->m_listStrColumn.GetHeadPosition();
		while (pos != NULL)
		{
			CString strItem = pParent->m_listStrColumn.GetNext(pos);
			wizdlg.m_listStrColumn.AddTail (strItem);
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
	m_cParam1.ReplaceSel (strStatement);
}



void CuDlgPropertyPageSqlExpressionAggregateParams::OnChangeEdit1() 
{
	EnableWizardButtons();
}

BOOL CuDlgPropertyPageSqlExpressionAggregateParams::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionAggregateParams::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlExpressionAggregateParams::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
		return APP_HelpInfo(m_hWnd, lpComp->GetID());
	return FALSE;
}

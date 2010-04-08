/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlecpar.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Comparaison page)
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
#include "sqlepsht.h"
#include "sqlecpar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionCompareParams, CPropertyPage)

CuDlgPropertyPageSqlExpressionCompareParams::CuDlgPropertyPageSqlExpressionCompareParams() : CPropertyPage(CuDlgPropertyPageSqlExpressionCompareParams::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionCompareParams)
	m_strParam1 = _T("");
	m_strParam2 = _T("");
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	m_strHelp3 = _T("");
	m_strParam1Name = _T("");
	m_strOperator = _T("");
	m_strParam2Name = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionCompareParams::~CuDlgPropertyPageSqlExpressionCompareParams()
{
}



void CuDlgPropertyPageSqlExpressionCompareParams::UpdateDisplay (LPGENFUNCPARAMS lpFparam)
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
	if (lpFparam->CaptionText2[0])
		m_strOperator.Format (_T("%s:"), lpFparam->CaptionText2);
	else
		m_strOperator =  _T("");
	if (lpFparam->CaptionText3[0])
		m_strParam2Name.Format (_T("%s:"), lpFparam->CaptionText3);
	else
		m_strParam2Name =  _T("");

	m_strParam1 = _T("");
	CheckRadioButton (IDC_RADIO1, IDC_RADIO6, IDC_RADIO1);
	UpdateData (FALSE);
}

void CuDlgPropertyPageSqlExpressionCompareParams::EnableWizardButtons()
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CString m_strText1;
	CString m_strText2;
	
	m_cParam1.GetWindowText (m_strText1);
	m_strText1.TrimLeft();
	m_strText1.TrimRight();
	m_cParam2.GetWindowText (m_strText2);
	m_strText2.TrimLeft();
	m_strText2.TrimRight();

	if (m_strText1.IsEmpty() || m_strText2.IsEmpty())
		pParent->SetWizardButtons(PSWIZB_DISABLEDFINISH|PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlExpressionCompareParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionCompareParams)
	DDX_Control(pDX, IDC_EDIT2, m_cParam2);
	DDX_Control(pDX, IDC_EDIT1, m_cParam1);
	DDX_Text(pDX, IDC_EDIT1, m_strParam1);
	DDX_Text(pDX, IDC_EDIT2, m_strParam2);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	DDX_Text(pDX, IDC_STATIC4, m_strHelp3);
	DDX_Text(pDX, IDC_STATIC5, m_strParam1Name);
	DDX_Text(pDX, IDC_STATIC6, m_strOperator);
	DDX_Text(pDX, IDC_STATIC7, m_strParam2Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionCompareParams, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionCompareParams)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1Param)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2Param)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEdit2)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionCompareParams message handlers

BOOL CuDlgPropertyPageSqlExpressionCompareParams::OnSetActive() 
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

BOOL CuDlgPropertyPageSqlExpressionCompareParams::OnWizardFinish() 
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
		// Compare operation alway have 3 arguments:
		CString strStatement;
		CString strOperator = GetOperator();
		strStatement.Format ((LPCTSTR)fparam.resultformat3, (LPCTSTR)m_strParam1, (LPCTSTR)strOperator, (LPCTSTR)m_strParam2);
		pParent->SetStatement (strStatement);
	}
	else
		pParent->SetStatement (NULL);

	return TRUE;
}

CString CuDlgPropertyPageSqlExpressionCompareParams::GetOperator()
{
	int nID = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO6);
	switch (nID)
	{
	case IDC_RADIO1:
		return _T(">");
	case IDC_RADIO2:
		return _T("<");
	case IDC_RADIO3:
		return _T(">=");
	case IDC_RADIO4:
		return _T("<=");
	case IDC_RADIO5:
		return _T("!=");
	case IDC_RADIO6:
		return _T("=");
	default:
		return _T(">");
	}
}

void CuDlgPropertyPageSqlExpressionCompareParams::OnButton1Param() 
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

void CuDlgPropertyPageSqlExpressionCompareParams::OnButton2Param() 
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
	m_cParam2.ReplaceSel (strStatement);
}

void CuDlgPropertyPageSqlExpressionCompareParams::OnChangeEdit1() 
{
	EnableWizardButtons();
}

void CuDlgPropertyPageSqlExpressionCompareParams::OnChangeEdit2() 
{
	EnableWizardButtons();
}

BOOL CuDlgPropertyPageSqlExpressionCompareParams::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionCompareParams::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlExpressionCompareParams::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
		return APP_HelpInfo(m_hWnd, lpComp->GetID());
	return FALSE;
}

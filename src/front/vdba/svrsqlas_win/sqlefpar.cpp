/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlefpar.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Function Parameters)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqlepsht.h"
#include "sqlwpsht.h"
#include "sqlefpar.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionFunctionParams, CPropertyPage)

CuDlgPropertyPageSqlExpressionFunctionParams::CuDlgPropertyPageSqlExpressionFunctionParams() : CPropertyPage(CuDlgPropertyPageSqlExpressionFunctionParams::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionFunctionParams)
	m_strParam1 = _T("");
	m_strParam2 = _T("");
	m_strParam3 = _T("");
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	m_strHelp3 = _T("");
	m_strParam1Name = _T("");
	m_strParam1Text = _T("");
	m_strParam1Op = _T("");
	m_strParam2Name = _T("");
	m_strParam2Text = _T("");
	m_strParam2Op = _T("");
	m_strParam3Name = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionFunctionParams::~CuDlgPropertyPageSqlExpressionFunctionParams()
{
}

void CuDlgPropertyPageSqlExpressionFunctionParams::UpdateDisplay (LPGENFUNCPARAMS lpFparam)
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
		m_strParam2Name.Format (_T("%s:"), lpFparam->CaptionText2);
	else
		m_strParam2Name =  _T("");
	if (lpFparam->CaptionText3[0])
		m_strParam3Name.Format (_T("%s:"), lpFparam->CaptionText3);
	else
		m_strParam3Name =  _T("");

	if (lpFparam->BetweenText1Left[0])
		m_strParam1Text = lpFparam->BetweenText1Left;
	else
		m_strParam1Text = _T("");
	if (lpFparam->BetweenText2Left[0])
		m_strParam2Text = lpFparam->BetweenText2Left;
	else
		m_strParam2Text = _T("");

	if (lpFparam->BetweenText1Right[0])
		m_strParam1Op = lpFparam->BetweenText1Right;
	else
		m_strParam1Op = _T("");
	if (lpFparam->BetweenText2Right[0])
		m_strParam2Op = lpFparam->BetweenText2Right;
	else
		m_strParam2Op = _T("");

	if (lpFparam->nbargsmax < 3)
	{
		m_cEdit3.ShowWindow (SW_HIDE);
		m_cButton3.ShowWindow (SW_HIDE);
	}
	if (lpFparam->nbargsmax < 2)
	{
		m_cEdit2.ShowWindow (SW_HIDE);
		m_cButton2.ShowWindow (SW_HIDE);
	}
	m_strParam1 = _T("");
	m_strParam2 = _T("");
	m_strParam3 = _T("");
	UpdateData (FALSE);
}

void CuDlgPropertyPageSqlExpressionFunctionParams::EnableWizardButtons()
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CString m_strText1;
	
	m_cEdit1.GetWindowText (m_strText1);
	m_strText1.TrimLeft();
	m_strText1.TrimRight();

	if (m_strText1.IsEmpty())
		pParent->SetWizardButtons(PSWIZB_DISABLEDFINISH|PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlExpressionFunctionParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionFunctionParams)
	DDX_Control(pDX, IDC_BUTTON3, m_cButton3);
	DDX_Control(pDX, IDC_BUTTON2, m_cButton2);
	DDX_Control(pDX, IDC_BUTTON1, m_cButton1);
	DDX_Control(pDX, IDC_EDIT3, m_cEdit3);
	DDX_Control(pDX, IDC_EDIT2, m_cEdit2);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strParam1);
	DDX_Text(pDX, IDC_EDIT2, m_strParam2);
	DDX_Text(pDX, IDC_EDIT3, m_strParam3);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	DDX_Text(pDX, IDC_STATIC4, m_strHelp3);
	DDX_Text(pDX, IDC_STATIC5, m_strParam1Name);
	DDX_Text(pDX, IDC_STATIC6, m_strParam1Text);
	DDX_Text(pDX, IDC_STATIC7, m_strParam1Op);
	DDX_Text(pDX, IDC_STATIC8, m_strParam2Name);
	DDX_Text(pDX, IDC_STATIC9, m_strParam2Text);
	DDX_Text(pDX, IDC_STATIC10, m_strParam2Op);
	DDX_Text(pDX, IDC_STATIC11, m_strParam3Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionFunctionParams, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionFunctionParams)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1Param)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2Param)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3Param)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionFunctionParams message handlers

BOOL CuDlgPropertyPageSqlExpressionFunctionParams::OnSetActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	CString strCapt = pParent->GetCaption();
	//
	//
	GENFUNCPARAMS fparam;
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
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


void CuDlgPropertyPageSqlExpressionFunctionParams::OnButton1Param() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (!lpComp)
		return;
	try
	{
		if (lpComp->GetID() == F_SUBSEL_SUBSEL)
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
					m_cEdit1.ReplaceSel(strStatement, TRUE);
			}
		}
		else
		{
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
			int nResult = wizdlg.DoModal();
			if (nResult == IDCANCEL)
				return;
			CString strStatement;
			wizdlg.GetStatement (strStatement);
			if (strStatement.IsEmpty())
				return;
			m_cEdit1.ReplaceSel (strStatement);
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
}

void CuDlgPropertyPageSqlExpressionFunctionParams::OnButton2Param() 
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
	m_cEdit2.ReplaceSel (strStatement);
}

void CuDlgPropertyPageSqlExpressionFunctionParams::OnButton3Param() 
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
	m_cEdit3.ReplaceSel (strStatement);
}

BOOL CuDlgPropertyPageSqlExpressionFunctionParams::OnWizardFinish() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	GENFUNCPARAMS fparam;
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
	{
		CString strStatement;
		memset (&fparam, 0, sizeof(fparam));
		FillFunctionParameters (&fparam, lpComp);
		UpdateData (TRUE);
		m_strParam1.TrimLeft();
		m_strParam2.TrimLeft();
		m_strParam3.TrimLeft();
		m_strParam1.TrimRight();
		m_strParam2.TrimRight();
		m_strParam3.TrimRight();

		switch (fparam.nbargsmax)
		{
		case 1:
			strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)m_strParam1);
			break;
		case 2:
			if (m_strParam2.IsEmpty())
				strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)m_strParam1);
			else
				strStatement.Format ((LPCTSTR)fparam.resultformat2, (LPCTSTR)m_strParam1, (LPCTSTR)m_strParam2);
			break;
		case 3:
			if (m_strParam2.IsEmpty() && m_strParam3.IsEmpty())
				strStatement.Format ((LPCTSTR)fparam.resultformat1, (LPCTSTR)m_strParam1);
			else
			if (!m_strParam2.IsEmpty() && !m_strParam3.IsEmpty())
				strStatement.Format ((LPCTSTR)fparam.resultformat3, (LPCTSTR)m_strParam1, (LPCTSTR)m_strParam2, (LPCTSTR)m_strParam3);
			else
			if (m_strParam2.IsEmpty())
				strStatement.Format ((LPCTSTR)fparam.resultformat2, (LPCTSTR)m_strParam1, (LPCTSTR)m_strParam3);
			else
				strStatement.Format ((LPCTSTR)fparam.resultformat2, (LPCTSTR)m_strParam1, (LPCTSTR)m_strParam2);
			break;
		default:
			break;
		}
		pParent->SetStatement (strStatement);
	}
	else
		pParent->SetStatement (NULL);
	return TRUE;
}

void CuDlgPropertyPageSqlExpressionFunctionParams::OnChangeEdit1() 
{
	EnableWizardButtons();
}

BOOL CuDlgPropertyPageSqlExpressionFunctionParams::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionFunctionParams::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlExpressionFunctionParams::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
		return APP_HelpInfo(m_hWnd, lpComp->GetID());
	return FALSE;
}

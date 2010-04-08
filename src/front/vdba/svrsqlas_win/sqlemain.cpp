/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlemain.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (main page: function choice)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
** 27-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "sqlepsht.h"
#include "sqlemain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionMain, CPropertyPage)

CuDlgPropertyPageSqlExpressionMain::CuDlgPropertyPageSqlExpressionMain() : CPropertyPage(CuDlgPropertyPageSqlExpressionMain::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionMain)
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionMain::~CuDlgPropertyPageSqlExpressionMain()
{
}


void CuDlgPropertyPageSqlExpressionMain::ConstructNextStep()
{
	if (!IsWindow(m_hWnd))
		return;
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	int i, nPageCount = pParent->GetPageCount();
	for (i=1; i<nPageCount; i++)
	{
		pParent->RemovePage (1);
	}
	int index = m_cListBox2.GetCurSel();
	if (index == LB_ERR)
		return;
	CaSQLComponent* lpComp = (CaSQLComponent*)m_cListBox2.GetItemData(index);
	if (!lpComp)
		return;

	switch (lpComp->GetID()) 
	{
	case F_AGG_ANY    :
	case F_AGG_COUNT  :
	case F_AGG_SUM    :
	case F_AGG_AVG    :
	case F_AGG_MAX    :
	case F_AGG_MIN    :
		pParent->SetStepAggregate();
		break;
	case F_PRED_COMP:
		pParent->SetStepComparaison();
		break;
	case F_PRED_ANY  :
	case F_PRED_ALL  :
	case F_PRED_EXIST:
		pParent->SetStepAnyAllExist();
		break;
	case F_DB_TABLES   :
	case F_DB_DATABASES:
	case F_DB_USERS    :
	case F_DB_GROUPS   :
	case F_DB_ROLES    :
	case F_DB_PROFILES :
	case F_DB_LOCATIONS:
	case OF_DB_DBAREA:
	case OF_DB_STOGROUP:
		pParent->SetStepDBObject();
		break;
	case F_DB_COLUMNS  :
		pParent->SetStepDBObject();
		break;
	case F_PRED_IN:
		pParent->SetStepIn();
		break;
	default:
		pParent->SetStepFunctionParam();
	}
}

DWORD CuDlgPropertyPageSqlExpressionMain::GetComponentCategory()
{
	int nSel = m_cListBox2.GetCurSel();
	if (nSel == LB_ERR)
		return 0;
	return m_cListBox2.GetItemData (nSel);
}


void CuDlgPropertyPageSqlExpressionMain::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionMain)
	DDX_Control(pDX, IDC_LIST2, m_cListBox2);
	DDX_Control(pDX, IDC_LIST1, m_cListBox1);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionMain, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionMain)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeList2)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionMain message handlers

BOOL CuDlgPropertyPageSqlExpressionMain::OnSetActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_NEXT);
	CString strCapt = pParent->GetCaption();
	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlExpressionMain::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CString strCapt = pParent->GetCaption();

	CTypedPtrList<CObList, CaSQLFamily*>& listSqlFamily = theApp.GetSqlFamily();
	POSITION pos = listSqlFamily.GetHeadPosition();

	int n2Sel = LB_ERR, i = 0, index = LB_ERR;
	while (pos != NULL)
	{
		CaSQLFamily* pFam = listSqlFamily.GetNext(pos);

		index = m_cListBox1.AddString (pFam->GetName());
		if (index != LB_ERR)
			m_cListBox1.SetItemData (index, (DWORD)pFam);
		if (pFam->GetID() == pParent->m_nFamilyID)
			n2Sel = i;
		i++;
	}
	//
	// Select the item specified by m_nCategory, and for the selection change:
	if (n2Sel == LB_ERR)
	{
		n2Sel = 0;
		pParent->m_nFamilyID = listSqlFamily.GetHead()->GetID();
	}
	m_cListBox1.SetCurSel (n2Sel);
	OnSelchangeList1() ;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlExpressionMain::OnSelchangeList1() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	
	int index = m_cListBox1.GetCurSel();
	if (index == LB_ERR)
		return;
	CaSQLFamily* lpFam = (CaSQLFamily*)m_cListBox1.GetItemData(index);
	ASSERT(lpFam);
	if (!lpFam)
		return;
	CTypedPtrList<CObList, CaSQLComponent*>* pListSqlComp = lpFam->GetListComponent();
	ASSERT(pListSqlComp);
	if (!pListSqlComp)
		return;

	pParent->m_nFamilyID = lpFam->GetID();
	m_cListBox2.ResetContent();
	POSITION pos = pListSqlComp->GetHeadPosition();
	while (pos != NULL)
	{
		CaSQLComponent* lpComp = pListSqlComp->GetNext(pos);
		index = m_cListBox2.AddString (lpComp->GetFunctionName());
		if (index != LB_ERR)
			m_cListBox2.SetItemData (index, (DWORD)lpComp);
	}

	//
	// Set the selection of Category Component to the first item:
	m_cListBox2.SetCurSel(0);

	//
	// Fetch the selected Component category
	index = m_cListBox2.GetCurSel();
	if (index == LB_ERR)
		return;
	CaSQLComponent* lpComp = (CaSQLComponent*)m_cListBox2.GetItemData(index);
	if (lpComp)
	{
		m_strFunctionName = lpComp->GetFullName();
		m_strHelp1.LoadString(lpComp->GetHelpID1());
		m_strHelp2 = _T("");
		UpdateData (FALSE);
		if (lpComp->HasArgument()) 
		{
			ConstructNextStep();
		}
		else
		{
			pParent->SetWizardButtons(PSWIZB_FINISH);
		}
	}
}

void CuDlgPropertyPageSqlExpressionMain::OnSelchangeList2() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	
	int index = m_cListBox1.GetCurSel();
	if (index == LB_ERR)
		return;
	CaSQLFamily* lpFam = (CaSQLFamily*)m_cListBox1.GetItemData(index);
	ASSERT(lpFam);
	if (!lpFam)
		return;
	CTypedPtrList<CObList, CaSQLComponent*>* pListSqlComp = lpFam->GetListComponent();
	ASSERT(pListSqlComp);
	if (!pListSqlComp)
		return;

	index = m_cListBox2.GetCurSel();
	if (index == LB_ERR)
		return;
	CaSQLComponent* lpComp = (CaSQLComponent*)m_cListBox2.GetItemData(index);
	ASSERT(lpComp);
	if (lpComp)
	{
		m_strFunctionName = lpComp->GetFullName();
		m_strHelp1.LoadString(lpComp->GetHelpID1());
		m_strHelp2 = _T("");
		UpdateData (FALSE);
		if (lpComp->HasArgument()) 
		{
			ConstructNextStep();
		}
		else
		{
			pParent->SetWizardButtons(PSWIZB_FINISH);
		}
	}
}

BOOL CuDlgPropertyPageSqlExpressionMain::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionMain::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlExpressionMain::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_EXPR_MAIN);
}

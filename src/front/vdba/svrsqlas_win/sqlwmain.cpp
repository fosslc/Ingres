/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlwmain.cpp, Implementation File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select, Insert, Update, Delete)
**               Main page.
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
#include "sqlwmain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardMain, CPropertyPage)

CuDlgPropertyPageSqlWizardMain::CuDlgPropertyPageSqlWizardMain() : CPropertyPage(CuDlgPropertyPageSqlWizardMain::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardMain)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_SQL_MAIN);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardMain::~CuDlgPropertyPageSqlWizardMain()
{
}

void CuDlgPropertyPageSqlWizardMain::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardMain)
	DDX_Control(pDX, IDC_IMAGE_OPENING, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardMain, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardMain)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1Select)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2Insert)
	ON_BN_CLICKED(IDC_RADIO3, OnRadio3Update)
	ON_BN_CLICKED(IDC_RADIO4, OnRadio4Delete)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardMain message handlers

void CuDlgPropertyPageSqlWizardMain::OnRadio1Select() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetCategorySelect();
}

void CuDlgPropertyPageSqlWizardMain::OnRadio2Insert() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetCategoryInsert();
}

void CuDlgPropertyPageSqlWizardMain::OnRadio3Update() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetCategoryUpdate();
}

void CuDlgPropertyPageSqlWizardMain::OnRadio4Delete() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetCategoryDelete();
}

BOOL CuDlgPropertyPageSqlWizardMain::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CheckRadioButton (IDC_RADIO1, IDC_RADIO4, IDC_RADIO1);
	OnRadio1Select();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuDlgPropertyPageSqlWizardMain::OnSetActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}

BOOL CuDlgPropertyPageSqlWizardMain::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlWizardMain::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardMain::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_MAIN);
}

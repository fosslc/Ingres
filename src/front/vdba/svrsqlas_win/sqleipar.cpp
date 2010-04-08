/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqleipar.cpp, Implementation File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**
**    Purpose  : Wizard for generate the sql expression (In Parameter page)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqlepsht.h"
#include "sqlwpsht.h"
#include "sqleipar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlExpressionInParams, CPropertyPage)

CuDlgPropertyPageSqlExpressionInParams::CuDlgPropertyPageSqlExpressionInParams() : CPropertyPage(CuDlgPropertyPageSqlExpressionInParams::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlExpressionInParams)
	m_strParam1 = _T("");
	m_strParam2 = _T("");
	m_strParam3 = _T("");
	m_strFunctionName = _T("");
	m_strHelp1 = _T("");
	m_strHelp2 = _T("");
	m_strHelp3 = _T("");
	//}}AFX_DATA_INIT
}

CuDlgPropertyPageSqlExpressionInParams::~CuDlgPropertyPageSqlExpressionInParams()
{
}


void CuDlgPropertyPageSqlExpressionInParams::UpdateDisplay (LPGENFUNCPARAMS lpFparam)
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


	m_strParam1 = _T("");
	m_strParam2 = _T("");
	m_strParam3 = _T("");
	//
	// In or Not to be In (In)
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	//
	// Sub Query or List (List)
	CheckRadioButton (IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);

	UpdateData (FALSE);
	m_cParam1.SetFocus();
}



void CuDlgPropertyPageSqlExpressionInParams::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlExpressionInParams)
	DDX_Control(pDX, IDC_LIST1, m_cListBox1);
	DDX_Control(pDX, IDC_EDIT3, m_cParam3);
	DDX_Control(pDX, IDC_EDIT2, m_cParam2);
	DDX_Control(pDX, IDC_EDIT1, m_cParam1);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonExp3);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonExp2);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonExp1);
	DDX_Control(pDX, IDC_BUTTON8, m_cButtonRemove);
	DDX_Control(pDX, IDC_BUTTON7, m_cButtonReplace);
	DDX_Control(pDX, IDC_BUTTON6, m_cButtonAdd);
	DDX_Control(pDX, IDC_BUTTON5, m_cButtonInsert);
	DDX_Text(pDX, IDC_EDIT1, m_strParam1);
	DDX_Text(pDX, IDC_EDIT2, m_strParam2);
	DDX_Text(pDX, IDC_EDIT3, m_strParam3);
	DDX_Text(pDX, IDC_STATIC1, m_strFunctionName);
	DDX_Text(pDX, IDC_STATIC2, m_strHelp1);
	DDX_Text(pDX, IDC_STATIC3, m_strHelp2);
	DDX_Text(pDX, IDC_STATIC4, m_strHelp3);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlExpressionInParams, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlExpressionInParams)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioIn)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioNotIn)
	ON_BN_CLICKED(IDC_RADIO3, OnRadioSubQuery)
	ON_BN_CLICKED(IDC_RADIO4, OnRadioList)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEdit3)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON5, OnButtonInsert)
	ON_BN_CLICKED(IDC_BUTTON6, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON7, OnButtonReplace)
	ON_BN_CLICKED(IDC_BUTTON8, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1Param)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2Param)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3Param)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlExpressionInParams message handlers

BOOL CuDlgPropertyPageSqlExpressionInParams::OnSetActive() 
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
	EnableControls();
	return CPropertyPage::OnSetActive();
}

//
// Enable control according to the Sub-query or List:
void CuDlgPropertyPageSqlExpressionInParams::EnableControls() 
{
	BOOL bSubquery = IsDlgButtonChecked (IDC_RADIO3);
	if (bSubquery) 
	{
		//m_cListBox1.ResetContent();
		m_cParam3.SetWindowText (_T(""));
	}
	else 
	{
		m_cParam2.SetWindowText (_T(""));
	}
	m_cParam2.EnableWindow (bSubquery);
	m_cButtonExp2.EnableWindow (bSubquery);
	m_cListBox1.EnableWindow (!bSubquery);
	m_cButtonExp3.EnableWindow (!bSubquery);
	m_cParam3.EnableWindow (!bSubquery);
	m_cButtonAdd.EnableWindow (!bSubquery);
	m_cButtonInsert.EnableWindow (!bSubquery);
	m_cButtonReplace.EnableWindow (!bSubquery);
	m_cButtonRemove.EnableWindow (!bSubquery);
}

//
// !! Do not use the MESSAGE MAP ON_CONTROL_RANGE !! 
// Bug in the release version of MFC
void CuDlgPropertyPageSqlExpressionInParams::OnRadioIn() 
{
	EnableControls();
}

void CuDlgPropertyPageSqlExpressionInParams::OnRadioNotIn() 
{
	EnableControls();
}

void CuDlgPropertyPageSqlExpressionInParams::OnRadioSubQuery() 
{
	EnableControls();
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnRadioList() 
{
	EnableControls();
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnChangeEdit3() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnSelchangeList1() 
{
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnButtonInsert() 
{
	CString strText3;
	m_cParam3.GetWindowText (strText3);
	strText3.TrimLeft();
	strText3.TrimRight();
	int nSel = m_cListBox1.GetCurSel();
	if (nSel == LB_ERR)
		nSel = 0;
	nSel = m_cListBox1.InsertString (nSel, (LPCTSTR)strText3);
	if (nSel != LB_ERR)
		m_cListBox1.SetCurSel (nSel);
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnButtonAdd() 
{
	CString strText3;
	m_cParam3.GetWindowText (strText3);
	strText3.TrimLeft();
	strText3.TrimRight();
	int nSel = m_cListBox1.AddString ((LPCTSTR)strText3);
	if (nSel != LB_ERR)
		m_cListBox1.SetCurSel (nSel);
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnButtonReplace() 
{
	CString strText3;
	m_cParam3.GetWindowText (strText3);
	strText3.TrimLeft();
	strText3.TrimRight();
	int nSel = m_cListBox1.GetCurSel();
	if (nSel == LB_ERR)
		return;
	m_cListBox1.DeleteString (nSel);
	nSel = m_cListBox1.InsertString(nSel, (LPCTSTR)strText3);
	if (nSel != LB_ERR)
		m_cListBox1.SetCurSel (nSel);
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::OnButtonRemove() 
{
	int nCount = m_cListBox1.GetCount();
	if (nCount == 0)
		return;
	int nSel = m_cListBox1.GetCurSel();
	if (nSel == LB_ERR)
		return;
	m_cListBox1.DeleteString (nSel);
	EnableButton4List();
}

void CuDlgPropertyPageSqlExpressionInParams::EnableButton4List()
{
	BOOL bEnableRemoveReplace = FALSE;
	BOOL bEnableAddInsert     = FALSE;
	CString strText3;

	int nCount = m_cListBox1.GetCount();
	if (nCount > 0)
	{
		if (m_cListBox1.GetCurSel() != LB_ERR)
			bEnableRemoveReplace = TRUE;
	}

	bEnableAddInsert = (m_cParam3.GetWindowTextLength() > 0)? TRUE: FALSE;
	if (bEnableAddInsert)
	{
		m_cParam3.GetWindowText (strText3);
		strText3.TrimLeft();
		strText3.TrimRight();
		if (strText3.IsEmpty())
			bEnableAddInsert = FALSE;
	}
	m_cButtonAdd.EnableWindow (bEnableAddInsert);
	m_cButtonInsert.EnableWindow (bEnableAddInsert);	
	m_cButtonRemove.EnableWindow (bEnableRemoveReplace);
	m_cButtonReplace.EnableWindow (bEnableRemoveReplace);	
}

BOOL CuDlgPropertyPageSqlExpressionInParams::OnWizardFinish() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	GENFUNCPARAMS fparam;
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
	{
		CString strOperant2;
		memset (&fparam, 0, sizeof(fparam));
		FillFunctionParameters (&fparam, lpComp);
		UpdateData (TRUE);
		int nID = GetCheckedRadioButton(IDC_RADIO3, IDC_RADIO4);
		if (nID == IDC_RADIO3)
			strOperant2 = m_strParam2;
		else
		{
			BOOL bOne = TRUE;
			CString strItem;
			int i, nCount = m_cListBox1.GetCount();
			strOperant2 = _T("");

			for (i=0; i<nCount; i++)
			{
				m_cListBox1.GetText(i, strItem);
				if (!bOne)
					strOperant2 += _T(", ");
				strOperant2 += strItem;
				bOne = FALSE;
			}
		}
		//
		// In function always has 3 arguments:
		CString strStatement;
		strStatement.Format ((LPCTSTR)fparam.resultformat3, (LPCTSTR)m_strParam1, _T("in"), (LPCTSTR)strOperant2);
		pParent->SetStatement (strStatement);
	}
	else
		pParent->SetStatement (NULL);
	return TRUE;
}

void CuDlgPropertyPageSqlExpressionInParams::OnButton1Param() 
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

void CuDlgPropertyPageSqlExpressionInParams::OnButton2Param() 
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

void CuDlgPropertyPageSqlExpressionInParams::OnButton3Param() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CxDlgPropertySheetSqlExpressionWizard wizdlg;
	wizdlg.m_queryInfo = pParent->m_queryInfo;
	wizdlg.m_nFamilyID    = FF_DATABASEOBJECTS;
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
	m_cParam3.ReplaceSel (strStatement);	
}

BOOL CuDlgPropertyPageSqlExpressionInParams::OnKillActive() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlExpressionInParams::OnCancel() 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlExpressionInParams::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CxDlgPropertySheetSqlExpressionWizard* pParent = (CxDlgPropertySheetSqlExpressionWizard*)GetParent();
	CuDlgPropertyPageSqlExpressionMain* pMainPage = &(pParent->m_PageMain);
	CaSQLComponent* lpComp = (CaSQLComponent*)pMainPage->GetComponentCategory();
	if (lpComp)
		return APP_HelpInfo(m_hWnd, lpComp->GetID());
	return FALSE;
}

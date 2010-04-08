/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwupd2.cpp, Implementation File 
**    Project  : Ingres II/Vdba. 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 2)
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
#include "sqlwupd2.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 2

IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardUpdate2, CPropertyPage)

CuDlgPropertyPageSqlWizardUpdate2::CuDlgPropertyPageSqlWizardUpdate2() : CPropertyPage(CuDlgPropertyPageSqlWizardUpdate2::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardUpdate2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_SQL_UPDATE2);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardUpdate2::~CuDlgPropertyPageSqlWizardUpdate2()
{
}


BOOL CuDlgPropertyPageSqlWizardUpdate2::GetListColumns (CaDBObject* pTable, CStringList& listColumn)
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CTypedPtrList<CObList, CaDBObject*> listObject;
	ASSERT(pTable);
	if (!pTable)
		return FALSE;
	CaLLQueryInfo info(pParent->m_queryInfo);
	if (pTable->GetObjectID() == OBT_TABLE)
		info.SetObjectType(OBT_TABLECOLUMN);
	else
		info.SetObjectType(OBT_VIEWCOLUMN);
	info.SetItem2(pTable->GetName(), pTable->GetOwner());
	BOOL bOk = theApp.INGRESII_QueryObject (&info, listObject);
	if (!bOk)
		return FALSE;

	CString strItem;
	POSITION pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = listObject.GetNext(pos);

		strItem.Format (
			_T("%s.%s.%s"),
			(LPCTSTR)INGRESII_llQuoteIfNeeded(pTable->GetOwner()),
			(LPCTSTR)INGRESII_llQuoteIfNeeded(pTable->GetName()),
			(LPCTSTR)INGRESII_llQuoteIfNeeded(pObj->GetName()));
		listColumn.AddTail(strItem);
		delete pObj;
	}

	return TRUE;
}


void CuDlgPropertyPageSqlWizardUpdate2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardUpdate2)
	DDX_Control(pDX, IDC_IMAGE_UPDATE, m_bitmap);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckSpecifyTable);
	DDX_Control(pDX, IDC_EDIT1, m_cEditCriteria);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardUpdate2, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardUpdate2)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonCriteria)
	ON_BN_CLICKED(IDC_CHECK1, OnSpecifyTables)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardUpdate2 message handlers

BOOL CuDlgPropertyPageSqlWizardUpdate2::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnInitDialog();
	VERIFY (m_cListTable.SubclassDlgItem (IDC_LIST1, this));
	VERIFY (m_cListView.SubclassDlgItem (IDC_LIST2, this));

	LSCTRLHEADERPARAMS2 lsp1[LAYOUT_NUMBER] =
	{
		{IDS_HEADER_TABLE, 120,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_OWNER,  80,  LVCFMT_LEFT, FALSE}
	};
	LSCTRLHEADERPARAMS2 lsp2[LAYOUT_NUMBER] =
	{
		{IDS_HEADER_VIEW,  120,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_OWNER,  80,  LVCFMT_LEFT, FALSE}
	};
	UINT nMask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	InitializeHeader2(&m_cListTable, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp1, LAYOUT_NUMBER);
	InitializeHeader2(&m_cListView,  LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp2, LAYOUT_NUMBER);

	//
	// Possible enhancement: personnalize according to database type
	m_ImageListTable.Create(IDB_TABLE, 16, 1, RGB(255, 0, 255));
	m_ImageListView.Create (IDB_VIEW,  16, 1, RGB(255, 0, 255));
	m_StateImageList.Create(IDB_STATEICONS, 16, 1, RGB(255, 0, 0));
	m_cListTable.SetImageList(&m_ImageListTable, LVSIL_SMALL);
	m_cListView.SetImageList(&m_ImageListView, LVSIL_SMALL);
	m_cListTable.SetImageList(&m_StateImageList, LVSIL_STATE);
	m_cListView.SetImageList(&m_StateImageList, LVSIL_STATE);

	try
	{
		CaLLQueryInfo info(pParent->m_queryInfo);
		info.SetObjectType(OBT_TABLE);
		SQLW_CuCheckListBoxFillTables  (&m_cListTable, &info);
		info.SetObjectType(OBT_VIEW);
		SQLW_CuCheckListBoxFillTables  (&m_cListView , &info);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		// _T("Cannot Query Tables and Views");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_TABLExVIEW, MB_ICONEXCLAMATION|MB_OK);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlWizardUpdate2::OnDestroy() 
{
	SQLW_CuCheckListBoxTablesClean (&m_cListTable);
	SQLW_CuCheckListBoxTablesClean (&m_cListView);
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardUpdate2::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

BOOL CuDlgPropertyPageSqlWizardUpdate2::OnSetActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);

	OnSpecifyTables();
	return CPropertyPage::OnSetActive();
}

void CuDlgPropertyPageSqlWizardUpdate2::OnButtonCriteria() 
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
		CString strItem;
		int i, nCount = m_cListTable.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListTable.GetItemChecked(i))
			{
				CaDBObject* pObj = (CaDBObject*)m_cListTable.GetItemData (i);
				CaDBObject* pNewObject = new CaDBObject(*pObj);
				pNewObject->SetObjectID(OBT_TABLE);
				wizdlg.m_listObject.AddTail (pNewObject);
			}
		}
		nCount = m_cListView.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListView.GetItemChecked(i))
			{
				CaDBObject* pObj = (CaDBObject*)m_cListView.GetItemData (i);
				CaDBObject* pNewObject = new CaDBObject(*pObj);
				pNewObject->SetObjectID(OBT_VIEW);
				wizdlg.m_listObject.AddTail (pNewObject);
			}
		}
		//
		// Append the current Table:
		CComboBox& comboTable = pParent->m_PageUpdate1.m_cComboTable;
		int nSel = comboTable.GetCurSel();
		if (nSel != CB_ERR)
		{
			CaDBObject* pObj = (CaDBObject*)comboTable.GetItemData (nSel);
			//
			// Check to see if the current table has already been added:
			BOOL b2Add = TRUE;
			POSITION pf = wizdlg.m_listObject.GetHeadPosition();
			while (pf != NULL)
			{
				CaDBObject* p = (CaDBObject*) wizdlg.m_listObject.GetNext (pf);
				if (p->GetName().CompareNoCase (pObj->GetName()) == 0 && p->GetOwner().CompareNoCase (pObj->GetOwner()) == 0)
				{
					b2Add = FALSE;
					break;
				}
			}
			if (b2Add)
			{
				CaDBObject* pNewObject = new CaDBObject(*pObj);
				if (pParent->m_PageUpdate1.IsDlgButtonChecked (IDC_RADIO1))
					pNewObject->SetObjectID(OBT_TABLE);
				else
					pNewObject->SetObjectID(OBT_VIEW);
				wizdlg.m_listObject.AddTail (pNewObject);
			}
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
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
	m_cEditCriteria.ReplaceSel (strStatement);
}

LRESULT CuDlgPropertyPageSqlWizardUpdate2::OnWizardNext() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	BOOL bMultiple = FALSE;

	m_listStrColumn.RemoveAll();
	BOOL bSpecify = (m_cCheckSpecifyTable.GetCheck() == 1)? TRUE: FALSE;
	if (!bSpecify)
	{
		//
		// Do not need to get the columns:
		return CPropertyPage::OnWizardNext();
	}
	int i, nCount1, nCount2 = 0;
	nCount1 = m_cListTable.GetItemCount();
	nCount2 = m_cListView.GetItemCount();
	bMultiple = (nCount1 + nCount2 > 1)? TRUE: FALSE;
	for (i=0; i<nCount1; i++)
	{
		if (m_cListTable.GetItemChecked(i))
		{
			CaDBObject* pTable = (CaDBObject*)m_cListTable.GetItemData(i);
			ASSERT(pTable);
			if (!pTable)
				continue;
			GetListColumns (pTable, m_listStrColumn);
		}
	}
	for (i=0; i<nCount2; i++)
	{
		if (m_cListView.GetItemChecked(i))
		{
			CaDBObject* pView = (CaDBObject*)m_cListView.GetItemData(i);
			ASSERT(pView);
			if (!pView)
				continue;

			GetListColumns (pView, m_listStrColumn);
		}
	}

	return CPropertyPage::OnWizardNext();
}

void CuDlgPropertyPageSqlWizardUpdate2::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

void CuDlgPropertyPageSqlWizardUpdate2::OnSpecifyTables() 
{
	int nShow = (m_cCheckSpecifyTable.GetCheck() == 1)? SW_SHOW: SW_HIDE;
	m_cListTable.ShowWindow (nShow);
	m_cListView.ShowWindow (nShow);
}

BOOL CuDlgPropertyPageSqlWizardUpdate2::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_UPD2);
}

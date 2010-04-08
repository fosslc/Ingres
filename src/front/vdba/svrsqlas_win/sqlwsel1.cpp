/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**    Source   : sqlwsel1.cpp, Implementation File
**
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select Page 1)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
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
#include "sqlwsel1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 2

IMPLEMENT_DYNCREATE(CuDlgPropertyPageSqlWizardSelect1, CPropertyPage)

CuDlgPropertyPageSqlWizardSelect1::CuDlgPropertyPageSqlWizardSelect1() : CPropertyPage(CuDlgPropertyPageSqlWizardSelect1::IDD)
{
	//{{AFX_DATA_INIT(CuDlgPropertyPageSqlWizardSelect1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nObjectSelected = 0;
	m_bitmap.SetBitmpapId (IDB_SQL_SELECT1);
	m_bitmap.SetCenterVertical(TRUE);
}

CuDlgPropertyPageSqlWizardSelect1::~CuDlgPropertyPageSqlWizardSelect1()
{
}


int CuDlgPropertyPageSqlWizardSelect1::CountSelectedObject()
{
	int i, nCheck = 0, nCount = m_cListTable.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListTable.GetItemChecked(i))
			nCheck++;
	}
	nCount = m_cListView.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListView.GetItemChecked(i))
			nCheck++;
	}
	return nCheck;
}

void CuDlgPropertyPageSqlWizardSelect1::EnableWizardButtons()
{
	m_nObjectSelected = CountSelectedObject();
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();

	if (m_nObjectSelected == 0)
		pParent->SetWizardButtons(PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
}


void CuDlgPropertyPageSqlWizardSelect1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageSqlWizardSelect1)
	DDX_Control(pDX, IDC_IMAGE_SELECT, m_bitmap);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckSystemObject);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CuDlgPropertyPageSqlWizardSelect1, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageSqlWizardSelect1)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckSyetemObject)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_LISTCTRL_CHECKCHANGE, OnListCtrlCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageSqlWizardSelect1 message handlers
long CuDlgPropertyPageSqlWizardSelect1::OnListCtrlCheckChange (WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = (HWND)wParam;
	if (hWnd == m_cListTable.m_hWnd || hWnd == m_cListView.m_hWnd)
		EnableWizardButtons();
	return 0;
}



BOOL CuDlgPropertyPageSqlWizardSelect1::OnSetActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	if (!pParent->IsSelectOnly())
		pParent->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	else
		pParent->SetWizardButtons(PSWIZB_NEXT);
	EnableWizardButtons();
	return CPropertyPage::OnSetActive();
}

void CuDlgPropertyPageSqlWizardSelect1::OnDestroy() 
{
	SQLW_CuCheckListBoxTablesClean (&m_cListTable);
	SQLW_CuCheckListBoxTablesClean (&m_cListView);
	CPropertyPage::OnDestroy();
}

BOOL CuDlgPropertyPageSqlWizardSelect1::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnInitDialog();
	VERIFY (m_cListTable.SubclassDlgItem (IDC_LIST1, this));
	VERIFY (m_cListView.SubclassDlgItem (IDC_LIST2, this));
	m_cListTable.SetLineSeperator(FALSE);
	m_cListView.SetLineSeperator(FALSE);

	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp1[LAYOUT_NUMBER] =
	{
		{IDS_HEADER_TABLE, 140,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_OWNER,  80,  LVCFMT_LEFT, FALSE}
	};
	LSCTRLHEADERPARAMS2 lsp2[LAYOUT_NUMBER] =
	{
		{IDS_HEADER_VIEW,  140,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_OWNER,  80,  LVCFMT_LEFT, FALSE}
	};
	UINT nMask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	InitializeHeader2(&m_cListTable, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp1, LAYOUT_NUMBER);
	InitializeHeader2(&m_cListView,  LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp2, LAYOUT_NUMBER);

	//
	// Possible enhancement: personnalize according to database type
	m_ImageListTable.Create(IDB_TABLE, 16, 1, RGB(255, 0, 255));
	m_ImageListView.Create (IDB_VIEW,  16, 1, RGB(255, 0, 255));
	m_StateImageList.Create(IDB_CHECK4LISTCTRL, 16, 1, RGB(255, 0, 0));
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
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return TRUE;
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
	EnableWizardButtons();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPropertyPageSqlWizardSelect1::OnCheckSyetemObject() 
{
	CWaitCursor doWaitCursor;
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	try
	{
		CTypedPtrList<CObList, CaDBObject*> listTable;
		CTypedPtrList<CObList, CaDBObject*> listView;

		CString strItem;
		int i, nCount;
		//
		// Store the checked items:

		nCount = m_cListTable.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListTable.GetItemChecked(i))
			{
				CaDBObject* pObj   =  (CaDBObject*)m_cListTable.GetItemData(i);
				CaDBObject* pTable = new CaDBObject(pObj->GetName(), pObj->GetOwner());
				listTable.AddTail (pTable);
			}
		}
		nCount = m_cListView.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListView.GetItemChecked(i))
			{
				CaDBObject* pObj   =  (CaDBObject*)m_cListTable.GetItemData(i);
				CaDBObject* pView = new CaDBObject(pObj->GetName(), pObj->GetOwner());
				listView.AddTail (pView);
			}
		}
		//
		// Cleanup list boxes:
		SQLW_CuCheckListBoxTablesClean (&m_cListTable);
		SQLW_CuCheckListBoxTablesClean (&m_cListView);
		
		if (m_cCheckSystemObject.GetCheck())
		{
			pParent->m_queryInfo.SetIncludeSystemObjects(TRUE);

			CaLLQueryInfo info(pParent->m_queryInfo);
			info.SetObjectType(OBT_TABLE);
			SQLW_CuCheckListBoxFillTables  (&m_cListTable, &info);
			info.SetObjectType(OBT_VIEW);
			SQLW_CuCheckListBoxFillTables  (&m_cListView, &info);
		}
		else
		{
			pParent->m_queryInfo.SetIncludeSystemObjects(FALSE);

			CaLLQueryInfo info(pParent->m_queryInfo);
			info.SetObjectType(OBT_TABLE);
			SQLW_CuCheckListBoxFillTables  (&m_cListTable, &info);
			info.SetObjectType(OBT_VIEW);
			SQLW_CuCheckListBoxFillTables  (&m_cListView, &info);
		}

		while (!listTable.IsEmpty())
		{
			CaDBObject* pObj = listTable.RemoveHead();
			SQLW_SelectObject (m_cListTable, pObj);
			delete pObj;
		}
		while (!listView.IsEmpty())
		{
			CaDBObject* pObj = listTable.RemoveHead();
			SQLW_SelectObject (m_cListView, pObj);
			delete pObj;
		}
		m_nObjectSelected = CountSelectedObject();
		EnableWizardButtons();
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
	catch (...)
	{
		// _T("Cannot Query Tables and Views");
		AfxMessageBox (IDS_MSG_FAIL_2_QUERY_TABLExVIEW, MB_ICONEXCLAMATION|MB_OK);
	}
}

BOOL CuDlgPropertyPageSqlWizardSelect1::OnKillActive() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	return CPropertyPage::OnKillActive();
}

void CuDlgPropertyPageSqlWizardSelect1::OnCancel() 
{
	CxDlgPropertySheetSqlWizard* pParent = (CxDlgPropertySheetSqlWizard*)GetParent();
	CPropertyPage::OnCancel();
}

BOOL CuDlgPropertyPageSqlWizardSelect1::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(m_hWnd, IDH_ASSISTANT_SEL1);
}

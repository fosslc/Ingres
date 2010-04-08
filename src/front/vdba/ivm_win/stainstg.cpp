/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : stainstg.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Instance (generic)
**
** History:
**
** 04-Mar-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 06-Mar-2000 (noifr01)
**    cosmetic string update, and moved the correponding string into 
**    resource
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 30-Oct-2003 (noifr01)
**    upon cleanup after massive ingres30->main propagation, added CR 
**    after traling }, which was causing problems in propagations
**/


#include "stdafx.h"
#include "ivm.h"
#include "mgrfrm.h"
#include "stainstg.h"
#include "ivmdisp.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusInstance dialog


CuDlgStatusInstance::CuDlgStatusInstance(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStatusInstance::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStatusInstance)
	m_bUnreadCriticalEvent = FALSE;
	//}}AFX_DATA_INIT
	m_pPage1 = NULL;
	m_pCurrentPage = NULL;
}


void CuDlgStatusInstance::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStatusInstance)
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Check(pDX, IDC_CHECK1, m_bUnreadCriticalEvent);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStatusInstance, CDialog)
	//{{AFX_MSG_MAP(CuDlgStatusInstance)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_COMPONENT_CHANGED, OnComponentChanged)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT, OnNewEvents)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusInstance message handlers

void CuDlgStatusInstance::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgStatusInstance::OnInitDialog() 
{
	CDialog::OnInitDialog();
		
	//
	// Construct the Tab Control:
	UINT iarray[1]= {IDS_INST_STARTHISTTAB};
	CString csTemp;

	CRect   r;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	for (int i=0; i<1; i++)
	{
		csTemp.LoadString(iarray[i]);
		item.pszText = (LPTSTR) (LPCTSTR) csTemp;
		m_cTab1.InsertItem (i, &item);
	}

	try
	{
		m_pPage1 = new CuDlgStatusInstallationTab1(&m_cTab1);
		m_pPage1->Create (IDD_STATUS_INSTALLATION_TAB1,&m_cTab1);
		m_pCurrentPage = m_pPage1;
		m_cTab1.SetCurSel (0);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuDlgStatusInstance::OnInitDialog): \nFailed to create the Tab of historiy of events"));
		return FALSE;
	}

	DisplayPage();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgStatusInstance::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	CRect r, rDlg;
	m_cTab1.GetWindowRect (r);
	ScreenToClient (r);
	GetClientRect (rDlg);
	r.right = rDlg.right - r.left;
	r.bottom = rDlg.bottom  - r.left;
	m_cTab1.MoveWindow (r);
	DisplayPage();
}


void CuDlgStatusInstance::DisplayPage()
{
	CRect r;
	int nSel = m_cTab1.GetCurSel();

	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		m_pCurrentPage->ShowWindow (SW_HIDE);
	switch (nSel)
	{
	case 0:
		m_pCurrentPage = m_pPage1;
		break;
	default:
		m_pCurrentPage = m_pPage1;
		break;
	}
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->MoveWindow (r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
	}
}


LONG CuDlgStatusInstance::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	try
	{
		CaTreeComponentItemData* pItem = NULL;
		CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
		if (pPageInfo)
			pItem = pPageInfo->GetIvmItem();
		//
		// Result from the Refresh by the Worker thread of Main application <CappIvmApplication>
		if (wParam == 0 && lParam == 0)
		{
			CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
			ASSERT (pManagerFrame);
			if (!pManagerFrame)
				return 0L;
			CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
			if (!pTreeView)
				return 0L;
			CTreeCtrl& cTree = pTreeView->GetTreeCtrl();
			HTREEITEM hSelected = cTree.GetSelectedItem();
			if (!hSelected)
				return 0L;
			pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelected);
			pPageInfo = pItem->GetPageInformation();
		}
		if (!pItem)
			return 0L;

		m_bUnreadCriticalEvent = (pItem->m_treeCtrlData.AlertGetCount() > 0)? TRUE: FALSE;
		UpdateData (FALSE);
		m_pPage1->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, wParam, lParam);
		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuDlgStatusInstance::OnUpdateData): \nFailed to refresh the Data"));
	}
	return 0L;
}


LONG CuDlgStatusInstance::OnComponentChanged (WPARAM wParam, LPARAM lParam)
{
	OnUpdateData (0, 0);
	return 0;
}

//
// There are new events, so refresh the pane:
LONG CuDlgStatusInstance::OnNewEvents (WPARAM wParam, LPARAM lParam)
{
	//
	// Construct the filter:
	CaTreeComponentItemData* pItem;
	CaEventFilter filter;
	CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pManagerFrame);
	if (!pManagerFrame)
		return 0;
	CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
	if (!pTreeView)
		return 0;
	CTreeCtrl& cTree = pTreeView->GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return 0;
	pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelected);
	CaPageInformation* pPageInfo = pItem? pItem->GetPageInformation(): NULL;

	if (pPageInfo && pItem)
	{
		OnUpdateData (0, (LPARAM)pPageInfo);
	}
	return 0L;
}

LONG CuDlgStatusInstance::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


LONG CuDlgStatusInstance::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStatusInstance::OnFind \n");
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		return (long)m_pCurrentPage->SendMessage (WMUSRMSG_FIND, wParam, lParam);
	}

	return 0L;
}

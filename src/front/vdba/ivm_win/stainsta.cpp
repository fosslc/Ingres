/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : stainsta.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01
**    Purpose  : Status Page for Installation branch 
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    Created
** 01-Feb-2000 (noifr01)
**    (SIR 100237) (reenabled the logic where IVM manages the fact that Ingres may be
**    started as a service or not) (removed some #ifdef OPING20)
** 01-Feb-2000 (uk$so01)
**    (Bug #100279)
**    Add the platform management, no ingres service and external access for WIN 95,98
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 31-Mar-2000 (uk$so01)
**    BUG 101126
**    Put the find dialog at top. When open the modal dialog, 
**    if the find dialog is opened, then close it.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 18-Apr-2001 (noifr01)
**    at the very first installation of IVM, have now the "Service mode" button pre-checked.
** 20-Jun-2001 (noifr01)
**    (bug 105065) renamed the 2 subtabs in the "status" right pane tab
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 16-Sep-2004 (uk$so01)
**    BUG #113060, IVM's checkbox “Service Mode” stays disable for a while (about 15 seconds) 
**    even if the Installation has been completely stopped.
**    Add a timer to monitor this checkbox every second, instead of waiting until the 
**    OnUpdateData has been called.
** 25-Jul-2007 (drivi01)
**	  Ingres will always start as a service on Vista.  Enable service 
**	  checkbox and gray it out so no one can change that.
**/
#include "stdafx.h"
#include "ivm.h"
#include "mgrfrm.h"
#include "stainsta.h"
#include "ivmdisp.h"
#include "xdgsysfo.h"
#include "regsvice.h"
#include "ivmdml.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgStatusInstallation::CuDlgStatusInstallation(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStatusInstallation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStatusInstallation)
	m_bUnreadCriticalEvent = FALSE;
	m_bStarted = FALSE;
	m_bStartedAll = FALSE;
	m_bStartedMore = FALSE;
	m_bAsService = TRUE;
	//}}AFX_DATA_INIT
	m_bAsService = theApp.m_setting.m_bAsService;
	if (theApp.m_bVista)
	{
		m_cCheckAsService.SetCheck(TRUE);
		m_bAsService = theApp.m_setting.m_bAsService = TRUE;
		m_cCheckAsService.EnableWindow(FALSE);
	}
	m_pPage1 = NULL;
	m_pPage2 = NULL;
	m_pCurrentPage = NULL;
	ServiceMode();
}


void CuDlgStatusInstallation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStatusInstallation)
	DDX_Control(pDX, IDC_CHECK4, m_cCheckAsService);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Check(pDX, IDC_CHECK1, m_bUnreadCriticalEvent);
	DDX_Check(pDX, IDC_CHECK5, m_bStarted);
	DDX_Check(pDX, IDC_CHECK6, m_bStartedAll);
	DDX_Check(pDX, IDC_CHECK7, m_bStartedMore);
	DDX_Check(pDX, IDC_CHECK4, m_bAsService);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStatusInstallation, CDialog)
	//{{AFX_MSG_MAP(CuDlgStatusInstallation)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSysInformation)
	ON_BN_CLICKED(IDC_CHECK4, OnCheckAsService)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_COMPONENT_CHANGED, OnComponentChanged)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT, OnNewEvents)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusInstallation message handlers

void CuDlgStatusInstallation::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgStatusInstallation::OnSize(UINT nType, int cx, int cy) 
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

BOOL CuDlgStatusInstallation::OnInitDialog() 
{
	CDialog::OnInitDialog();
/*#if !defined (OPING20) || defined (MAINWIN)*/
#if defined (MAINWIN)
	m_cCheckAsService.ShowWindow (SW_HIDE);
#else
	DWORD dwPlatform = theApp.GetPlatformID();
	if (dwPlatform == VER_PLATFORM_WIN32_WINDOWS)
	{
		m_cCheckAsService.ShowWindow (SW_HIDE);
		CWnd* pWnd = GetDlgItem (IDC_BUTTON1); // External access
		if (pWnd  && IsWindow (pWnd->m_hWnd))
			pWnd->ShowWindow (SW_HIDE);
	}
#endif
	//
	// Construct the Tab Control:
	// to be moved to the resource file - currently locked by (bigger) concurrent change
	TCHAR tchszTab [2][48] = {_T("Start/Stop Event History"), _T("Start/Stop Output Log")};

	CRect   r;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	for (int i=0; i<2; i++)
	{
		if (theApp.m_bVista && i == 1)
			break;
		item.pszText = (LPTSTR)tchszTab[i];
		m_cTab1.InsertItem (i, &item);
	}


	try
	{
		m_pPage1 = new CuDlgStatusInstallationTab1(&m_cTab1);
		m_pPage2 = new CuDlgStatusInstallationTab2(&m_cTab1);
		m_pPage1->Create (IDD_STATUS_INSTALLATION_TAB1,&m_cTab1);
		if (!theApp.m_bVista)
		m_pPage2->Create (IDD_STATUS_INSTALLATION_TAB2,&m_cTab1);
		m_pCurrentPage = m_pPage1;
		m_cTab1.SetCurSel (0);
		DisplayPage();
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuDlgStatusInstallation::OnInitDialog): \nFailed to create the Tab of historiy of events"));
		return FALSE;
	}

	SetTimer (m_nTimer1, 1000L, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CuDlgStatusInstallation::DisplayPage()
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
	case 1:
		m_pCurrentPage = m_pPage2;
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

void CuDlgStatusInstallation::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	DisplayPage();
	*pResult = 0;
}

LONG CuDlgStatusInstallation::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
		m_bStarted = FALSE;
		m_bStartedAll = FALSE;
		m_bStartedMore = FALSE;
		int nStatus = pItem->GetItemStatus();
		switch (nStatus)
		{
		case COMP_NORMAL:
			break;
		case COMP_START:
			m_bStarted = TRUE;
			m_bStartedAll = FALSE;
			break;
		case COMP_STARTMORE:
			m_bStarted = TRUE;
			m_bStartedMore = TRUE;
			break;
		case COMP_HALFSTART:
			m_bStarted = TRUE;
			m_bStartedAll = FALSE;
			break;
		default:
			break;
		}
		CaIvmTree& treeData = theApp.GetTreeData();
		m_bStartedMore = treeData.IsMultipleInstancesRunning();
		UpdateData (FALSE);
		m_pPage1->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, wParam, lParam);
		m_pPage2->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, wParam, lParam);

		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuDlgStatusInstallation::OnUpdateData): \nFailed to refresh the Data"));
	}
	return 0L;
}


LONG CuDlgStatusInstallation::OnComponentChanged (WPARAM wParam, LPARAM lParam)
{
	OnUpdateData (0, 0);
	return 0;
}

//
// There are new events, so refresh the pane:
LONG CuDlgStatusInstallation::OnNewEvents (WPARAM wParam, LPARAM lParam)
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
	
	//
	// Activate the Last start/stop output:
	if ((int)wParam == 1)
	{
		int nCurSel = m_cTab1.GetCurSel();
		if (nCurSel != 1)
		{
			m_cTab1.SetCurSel(1);
			DisplayPage();
		}
	}

	if (pPageInfo && pItem)
	{
		OnUpdateData (0, (LPARAM)pPageInfo);
	}
	return 0L;
}

void CuDlgStatusInstallation::OnButtonSysInformation() 
{
	if (!IVM_IsNameServerStarted())
	{
		//_T("The Name Server must be running for this operation");
		CString strMsg;
		strMsg.LoadString(IDS_MSG_IIGCN_MUSTBE_RUNNING);
		AfxMessageBox (strMsg);
		return;
	}

	CaModalUp modalUp;
	CxDlgSystemInfo dlg;
	//
	// Initialize the dlg public member here:

	dlg.DoModal();
}

void CuDlgStatusInstallation::ServiceMode()
{
#if !defined (MAINWIN)
	BOOL bStartedStop = FALSE;
	DWORD dwPlatform = theApp.GetPlatformID();
	if (dwPlatform == VER_PLATFORM_WIN32_WINDOWS)
	{
		theApp.m_setting.m_bAsService = FALSE;
		m_bAsService = FALSE;
		return;
	}

	BOOL bNameServerStarted = IVM_IsNameServerStarted();
	if (bNameServerStarted)
	{
		BOOL bService = FALSE;
		bService = IVM_IsServiceInstalled (theApp.m_strIngresServiceName);
		if (bService)
			bService = IVM_IsServiceRunning (theApp.m_strIngresServiceName);

		if (bService != m_bAsService)
		{
			//
			// Service mode has changed, save it to the .ini file:
			m_bAsService = bService;
			theApp.m_setting.m_bAsService = m_bAsService;
			theApp.m_setting.Save();
		}

		if (IsWindow (m_cCheckAsService.m_hWnd))
			m_cCheckAsService.EnableWindow (FALSE);
	}
	else
	{
		if (!theApp.m_bVista)
		{		
			if (IsWindow (m_cCheckAsService.m_hWnd))
				m_cCheckAsService.EnableWindow (TRUE);
		}
		else
		{
			BOOL bService = FALSE;
			bService = IVM_IsServiceInstalled(theApp.m_strIngresServiceName);
			m_bAsService = bService = theApp.m_setting.m_bAsService = TRUE;
			theApp.m_setting.Save();

			if (IsWindow (m_cCheckAsService.m_hWnd))
				m_cCheckAsService.EnableWindow (FALSE);
		}
	}
#endif
}

void CuDlgStatusInstallation::OnCheckAsService() 
{
	int nCheck = m_cCheckAsService.GetCheck();
	m_bAsService = (nCheck == 1)? TRUE: FALSE;
	theApp.m_setting.m_bAsService = m_bAsService;
	theApp.m_setting.Save();
}


LONG CuDlgStatusInstallation::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


LONG CuDlgStatusInstallation::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgParameterInstallation::OnFind \n");
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		return (long)m_pCurrentPage->SendMessage (WMUSRMSG_FIND, wParam, lParam);
	}

	return 0L;
}
void CuDlgStatusInstallation::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == m_nTimer1)
		ServiceMode();
	CDialog::OnTimer(nIDEvent);
}

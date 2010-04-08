/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cachetab.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut 
**    Purpose  : Modeless Dialog of the right pane of DBMS Cache 
**               It holds thow pages (parameter, derived)
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**
**/

#include "stdafx.h"
#include "vcbf.h"
#include "crightdg.h"
#include "cachefrm.h"
#include "cachetab.h"
#include "cachelst.h"
#include "cachevi1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheTab dialog
CuCacheCheckListCtrl* CuDlgCacheTab::GetCacheListCtrl()
{
	CWnd* pParent1 = GetParent ();              // View (CvDbmsCacheViewRight)
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // SplitterWnd
	ASSERT_VALID (pParent2);
	CWnd* pParent3 = pParent2->GetParent ();    // Frame (CfDbmsCacheFrame)
	ASSERT_VALID (pParent3);
	if (!((CfDbmsCacheFrame*)pParent3)->IsAllViewsCreated())
		return NULL;

	CvDbmsCacheViewLeft* pView = (CvDbmsCacheViewLeft*)((CSplitterWnd*)pParent2)->GetPane (0, 0);
	ASSERT_VALID (pView);
	return pView->GetCacheListCtrl();
}


CuDlgCacheTab::CuDlgCacheTab(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgCacheTab::IDD, pParent)
{
	m_pCurrentPage    = NULL;
	m_pCacheParameter = NULL;
	m_pCacheDerived   = NULL;

	//{{AFX_DATA_INIT(CuDlgCacheTab)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgCacheTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgCacheTab)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgCacheTab, CDialog)
	//{{AFX_MSG_MAP(CuDlgCacheTab)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheTab message handlers

void CuDlgCacheTab::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}

void CuDlgCacheTab::OnDestroy() 
{
	CDialog::OnDestroy();
}

BOOL CuDlgCacheTab::OnInitDialog() 
{
	CDialog::OnInitDialog();
	//
	// Initialize the tab control
	CRect   r;
	TC_ITEM item;
	CString strTab;
	UINT nTabID[2]= {IDS_TAB_CONFRIGHT_PARAMETERS, IDS_TAB_CONFRIGHT_DERIVED};
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	item.iImage     = -1;
	CTabCtrl* pTab = (CTabCtrl*)GetDlgItem(IDC_TAB1);
	ASSERT(pTab);
	for (int i=0; i<2; i++)  
	{
		strTab.LoadString (nTabID [i]);
		item.pszText = (LPTSTR)(LPCTSTR)strTab;
		pTab->InsertItem (i, &item);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgCacheTab::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CTabCtrl* pTab = (CTabCtrl*)GetDlgItem (IDC_TAB1);
	if (!(pTab && IsWindow (pTab->m_hWnd)))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	pTab->MoveWindow (rDlg);
	if (!m_pCurrentPage) 
		return;
	pTab->GetClientRect (r);
	pTab->AdjustRect (FALSE, r);
	m_pCurrentPage->MoveWindow(r);
	m_pCurrentPage->ShowWindow(SW_SHOW);
}

void CuDlgCacheTab::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	try
	{
		CuCacheCheckListCtrl* pList = GetCacheListCtrl();
		CACHELINEINFO* pData = NULL;
		UINT nState = 0;
		int i, nCurSel = -1, nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			nState = pList->GetItemState (i, LVIS_SELECTED);
			if (nState & LVIS_SELECTED)
			{
				nCurSel = i;
				break;
			}
		}
		if (nCurSel != -1)
			pData = (CACHELINEINFO*)pList->GetItemData (nCurSel);
		DisplayPage (pData);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgCacheTab::OnSelchangeTab1 has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (...)
	{
		TRACE0 ("CuDlgCacheTab::OnSelchangeTab1 has caught exception... \n");
	}
	*pResult = 0;
}


void CuDlgCacheTab::DisplayPage (CACHELINEINFO* pData)
{
	CString csButtonTitle;
	CWnd* pParent1 = GetParent ();              // View (CvDbmsCacheViewRight)
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // SplitterWnd
	ASSERT_VALID (pParent2);
	CWnd* pParent3 = pParent2->GetParent ();    // Frame (CfDbmsCacheFrame)
	ASSERT_VALID (pParent3);
	CWnd* pParent4 = pParent3->GetParent ();    // Modeless Dialog (CuDlgDbmsCache)
	ASSERT_VALID (pParent4);

	CWnd* pParent5 = pParent4->GetParent ();    // CTabCtrl
	ASSERT_VALID (pParent5);
	CWnd* pParent6 = pParent5->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent6);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent6)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent6)->GetDlgItem (IDC_BUTTON2);

	CuCacheCheckListCtrl* pList = GetCacheListCtrl();
	CTabCtrl* pTab = (CTabCtrl*)GetDlgItem(IDC_TAB1);
	if (m_pCurrentPage)
	{
		//
		// Try to save (and validate) the data that has been changed
		// in the old page before displaying a new page.
		m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
		m_pCurrentPage->ShowWindow (SW_HIDE);
		m_pCurrentPage = NULL;
	}
	if (!(pTab && IsWindow (pTab->m_hWnd)))
		return;

	UINT nState = 0;
	int i, nCurSel = -1, nCount = pList->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = pList->GetItemState (i, LVIS_SELECTED);
		if (nState & LVIS_SELECTED)
		{
			nCurSel = i;
			break;
		}
	}
	if (nCurSel == -1)
		return;
	if (!pData)
		return;
	try
	{
		if (!VCBFllInitCacheParms(pData))
			return;
		int nSel = pTab->GetCurSel();
		switch (nSel)
		{
		case 0:
			csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
			pButton1->SetWindowText (csButtonTitle);

			csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
			pButton2->SetWindowText (csButtonTitle);
			if (!m_pCacheParameter)
			{
				m_pCacheParameter = new CuDlgDbmsCacheParameter (pTab);
				m_pCacheParameter->Create (IDD_DBMS_PAGE_CACHE_PARAMETER, pTab);
			}
			m_pCurrentPage = m_pCacheParameter;
			break;
		case 1:
			csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
			pButton1->SetWindowText (csButtonTitle);

			csButtonTitle.LoadString(IDS_BUTTON_RECALCULATE);
			pButton2->SetWindowText (csButtonTitle);

			if (!m_pCacheDerived)
			{
				m_pCacheDerived = new CuDlgDbmsCacheDerived (pTab);
				m_pCacheDerived->Create (IDD_DBMS_PAGE_CACHE_DERIVED, pTab);
			}
			m_pCurrentPage = m_pCacheDerived;
			break;
		default:
			break;
		}
		if (m_pCurrentPage) 
		{
			CRect r;
			pTab->GetClientRect (r);
			pTab->AdjustRect (FALSE, r);
			m_pCurrentPage->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
			m_pCurrentPage->MoveWindow(r);
			m_pCurrentPage->ShowWindow(SW_SHOW);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgCacheTab::DisplayPage has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

LONG CuDlgCacheTab::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	return 0;
}

LONG CuDlgCacheTab::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);
	return 0;
}

LONG CuDlgCacheTab::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgCacheTab::OnButton1...\n");
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON1, 0, 0);
	return 0L;
}

LONG CuDlgCacheTab::OnButton2 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgCacheTab::OnButton2...\n");
	if (m_pCurrentPage)
		m_pCurrentPage->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON2, 0, 0);
	return 0L;
}

LONG CuDlgCacheTab::OnButton3 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgCacheTab::OnButton3...\n");
	return 0L;
}

UINT CuDlgCacheTab::GetHelpID()
{
	if (!m_pCurrentPage)
		return 0;
	CTabCtrl* pTab = (CTabCtrl*)GetDlgItem(IDC_TAB1);
	int nSel = pTab->GetCurSel();
	switch (nSel)
	{
	case 0:
		return IDD_DBMS_PAGE_CACHE_PARAMETER;
	case 1:
		return IDD_DBMS_PAGE_CACHE_DERIVED;
	default:
		return 0;
	}

	return 0;
}

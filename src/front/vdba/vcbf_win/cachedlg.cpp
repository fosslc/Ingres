/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cachedlg.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Modeless Dialog-DBMS Cache page
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 04-Jun-2002: (uk$so01) 
**    (BUG #107927)
**    Show the bottons "Edit Value" and "Restore" for this tab.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/

#include "stdafx.h"
#include "vcbf.h"
#include "cachedlg.h"
#include "crightdg.h"
#include "wmusrmsg.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LAYOUT_NUMBER 2
/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsCache dialog

CuDlgDbmsCache::CuDlgDbmsCache(CWnd* pParent)
	: CDialog(CuDlgDbmsCache::IDD, pParent)
{
	m_pCacheFrame     = NULL;
	//{{AFX_DATA_INIT(CuDlgDbmsCache)
	//}}AFX_DATA_INIT
}


void CuDlgDbmsCache::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDbmsCache)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDbmsCache, CDialog)
	//{{AFX_MSG_MAP(CuDlgDbmsCache)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
END_MESSAGE_MAP()


CvDbmsCacheViewLeft* CuDlgDbmsCache::GetLeftPane ()
{
	return m_pCacheFrame? (CvDbmsCacheViewLeft*)m_pCacheFrame->GetLeftPane(): NULL;
}

CvDbmsCacheViewRight* CuDlgDbmsCache::GetRightPane()
{
	return m_pCacheFrame? (CvDbmsCacheViewRight*)m_pCacheFrame->GetRightPane(): NULL;
}



/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsCache message handlers

BOOL CuDlgDbmsCache::OnInitDialog() 
{
	CRect    r;
	GetClientRect (r);
	 	
	m_pCacheFrame	= new CfDbmsCacheFrame();
	m_pCacheFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this);
	m_pCacheFrame->ShowWindow (SW_SHOW);
	m_pCacheFrame->InitialUpdateFrame (NULL, TRUE);

	return TRUE;
}



void CuDlgDbmsCache::OnDestroy() 
{
	CDialog::OnDestroy();
}



void CuDlgDbmsCache::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


void CuDlgDbmsCache::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	//
	// Resize the frame Window.

	if (m_pCacheFrame && IsWindow(m_pCacheFrame->m_hWnd)) 
	{
		CRect r;
		GetClientRect(r);
		m_pCacheFrame->MoveWindow(r);
	}
}

LONG CuDlgDbmsCache::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsCache::OnUpdateData...\n");
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);
	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);
	pButton1->EnableWindow (FALSE);
	pButton2->EnableWindow (FALSE);
	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);

	CvDbmsCacheViewLeft* pViewLeft = GetLeftPane ();
	if (pViewLeft)
		pViewLeft->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
	CvDbmsCacheViewRight* pViewRight = GetRightPane ();
	if (pViewRight)
		pViewRight->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);

	return 0L;
}

LONG CuDlgDbmsCache::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsCache::OnValidateData...\n");
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);
	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);
	pButton1->EnableWindow (FALSE);
	pButton2->EnableWindow (FALSE);

	CvDbmsCacheViewLeft* pViewLeft = GetLeftPane ();
	if (pViewLeft)
		pViewLeft->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);
	CvDbmsCacheViewRight* pViewRight = GetRightPane ();
	if (pViewRight)
		pViewRight->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);

	return 0L;
}

LONG CuDlgDbmsCache::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgDbmsCache::OnButton1...\n");
	CvDbmsCacheViewLeft* pViewLeft = GetLeftPane ();
	if (pViewLeft)
		pViewLeft->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON1, 0, 0);
	CvDbmsCacheViewRight* pViewRight = GetRightPane ();
	if (pViewRight)
		pViewRight->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON1, 0, 0);
	return 0L;
}

LONG CuDlgDbmsCache::OnButton2 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgDbmsCache::OnButton2...\n");
	CvDbmsCacheViewLeft* pViewLeft = GetLeftPane ();
	if (pViewLeft)
		pViewLeft->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON2, 0, 0);
	CvDbmsCacheViewRight* pViewRight = GetRightPane ();
	if (pViewRight)
		pViewRight->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON2, 0, 0);	
	return 0L;
}

LONG CuDlgDbmsCache::OnButton3 (UINT wParam, LONG lParam)
{
	CvDbmsCacheViewLeft* pViewLeft = GetLeftPane ();
	if (pViewLeft)
		pViewLeft->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON3, 0, 0);
	CvDbmsCacheViewRight* pViewRight = GetRightPane ();
	if (pViewRight)
		pViewRight->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON3, 0, 0);
	return 0L;
}






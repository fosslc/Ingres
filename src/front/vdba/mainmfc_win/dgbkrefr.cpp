/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgbkrefr.cpp, Implementation File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut
**    Purpose  : Modeless Dialog Box for Background Refresh Info 
**
** History:
** xx-Nov-1998 (uk$so01)
**    Created
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgbkrefr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" HWND DlgBkRefresh_Create(HWND hwndParent)
{
	try
	{
		CWnd* pWnd = CWnd::FromHandle(hwndParent);
		CuDlgBackgroundRefresh* pDlg = new CuDlgBackgroundRefresh(pWnd);
		pDlg->Create (IDD_BACKGROUND_REFRESH, pWnd);
		return pDlg->m_hWnd;
	}
	catch (...)
	{
		CString strMsg = _T("CuDlgBackgroundRefresh->Cannot Create Background Refresh Dialog\n");
		TRACE0 (strMsg);
	}
	return NULL;
}

extern "C" void DlgBkRefresh_SetInfo (HWND hWndDlg, LPCTSTR lpszText)
{
	::SendMessage (hWndDlg, WM_QUERY_UPDATING, 0, (LPARAM)lpszText);
}

CuDlgBackgroundRefresh::CuDlgBackgroundRefresh(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgBackgroundRefresh::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgBackgroundRefresh)
	m_strStatic1 = _T("");
	//}}AFX_DATA_INIT
	m_bAnimateOK = FALSE;
}


void CuDlgBackgroundRefresh::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgBackgroundRefresh)
	DDX_Control(pDX, IDC_ANIMATE1, m_cAnimate1);
	DDX_Text(pDX, IDC_MFC_STATIC1, m_strStatic1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgBackgroundRefresh, CDialog)
	//{{AFX_MSG_MAP(CuDlgBackgroundRefresh)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_QUERY_UPDATING, OnUpdateInfo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgBackgroundRefresh message handlers

void CuDlgBackgroundRefresh::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgBackgroundRefresh::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_bAnimateOK = m_cAnimate1.Open(IDR_ANCLOCK)? TRUE: FALSE;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgBackgroundRefresh::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (!m_bAnimateOK)
		return;
	if (bShow)
		m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
	else
		m_cAnimate1.Stop();
}

LONG CuDlgBackgroundRefresh::OnUpdateInfo (WPARAM wParam, LPARAM lParam)
{
	m_strStatic1 = lParam? (LPCTSTR)lParam: _T("");
	UpdateData (FALSE);
	return 0L;
}

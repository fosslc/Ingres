/*
**  Copyright (c) 2003 Ingres Corporation
*/

/*
**  Name: WaitDlg.cpp : implementation file
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
**	06-Apr-2010 (drivi01)
**	    Update OnTimer to take UINT_PTR as a parameter as UINT_PTR
**	    will be unsigned int on x86 and unsigned int64 on x64.
**	    This will cleanup warnings.
*/

#include "stdafx.h"
#include "IngClean.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

CWaitDlg::CWaitDlg(CString csText, UINT nAVI_Res /*=IDR_CLOCK_AVI*/, UINT nDialogID /*=IDD_WAIT_DIALOG*/, CWnd* pParent /*=NULL*/)
	: CDialog(CWaitDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWaitDlg)
		// NOTE: the ClassWizard will add member initialization here

	m_nAVI_Res=nAVI_Res;
	m_csText=csText;
	m_hThread=NULL;
	//}}AFX_DATA_INIT
}

void CWaitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaitDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaitDlg, CDialog)
	//{{AFX_MSG_MAP(CWaitDlg)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CWnd *hAniWnd=GetDlgItem(IDC_AVI);
	if (hAniWnd)
	{
		hAniWnd->SendMessage(ACM_OPEN, 0, (LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_nAVI_Res)));
		hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1), (LPARAM)MAKELONG(0, 0xFFFF));
	}

	CWnd *hTextWnd=GetDlgItem(IDC_TEXT);
	if (hTextWnd)
		hTextWnd->SetWindowText(m_csText);

	if (m_hThread)
		SetTimer(1, 20000, 0);

	CenterWindow(GetDesktopWindow());
	BringWindowToTop();
	ShowWindow(SW_SHOW);
	RedrawWindow();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWaitDlg::OnTimer(UINT_PTR nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == 1)
	{
		DWORD dwExitCode;
		
		if ((m_hThread) && (GetExitCodeThread(m_hThread, &dwExitCode)))
		{
			if (dwExitCode != STILL_ACTIVE)
				CDialog::OnCancel();
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CWaitDlg::OnClose() 
{
	//disable ALT+F4,
	return;
	//CDialog::OnClose();
}

BOOL CWaitDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSCOMMAND)
	{
		switch(pMsg->wParam)
		{
		case VK_ESCAPE: 
		case VK_RETURN: 
		case VK_CANCEL: return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: WaitDlg.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog


CWaitDlg::CWaitDlg(UINT uiStrText, UINT uiAVI_Resource /*=IDR_CLOCK_AVI*/, CWnd* pParent /*=NULL*/)
	: CDialog(CWaitDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWaitDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_uiAVI_Resource=uiAVI_Resource;
	m_strText.LoadString(uiStrText);
	m_hThreadWait=NULL;
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CWnd *hAniWnd=GetDlgItem(IDC_AVI);
	if(hAniWnd)
	{
		hAniWnd->SendMessage(ACM_OPEN, 0, (LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_uiAVI_Resource)));
		hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1), (LPARAM)MAKELONG(0, 0xFFFF));
	}
	
	CWnd *hTextWnd=GetDlgItem(IDC_TEXT);
	if(hTextWnd)
		hTextWnd->SetWindowText(m_strText);
	
    if(m_hThreadWait)
		SetTimer(1, 20000, 0);
	
	CenterWindow(GetDesktopWindow());
	BringWindowToTop();
	ShowWindow(SW_SHOW);
	RedrawWindow();
	
	return TRUE;
}

void CWaitDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent==1)
	{ 
		DWORD dwRet;
		
		if((m_hThreadWait) && (GetExitCodeThread(m_hThreadWait,&dwRet)))
		{ 
			if(dwRet!=STILL_ACTIVE)
				CDialog::OnCancel();
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

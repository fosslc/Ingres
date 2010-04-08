/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
******************************************************************************/

/******************************************************************************
**
** WaitDlg.cpp : implementation file
**
** History:
**	22-nov-98 (cucjo01)
**	    Added multiple transaction log file support
**	    Added animations during creation of log files
**
******************************************************************************/

#include "stdafx.h"
#include "vcbf.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

CWaitDlg::CWaitDlg(CString strText, UINT uiAVI_Resource, UINT uiDialogID /* = IDD_WAIT_DLG */, CWnd* pParent /* = NULL */)
	: CDialog(CWaitDlg::IDD, pParent)
{
    m_uiAVI_Resource = uiAVI_Resource;
    m_strText = strText;

    Create(uiDialogID);
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() 
{
  CDialog::OnInitDialog();

  CWnd *hAniWnd = GetDlgItem(IDC_AVI);
  if (hAniWnd)
  { hAniWnd->SendMessage(ACM_OPEN, 0, (LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_uiAVI_Resource)));
    hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1), (LPARAM)MAKELONG(0, 0xFFFF));
    
  }
 
  CWnd *hTextWnd = GetDlgItem(IDC_TEXT);
  if (hTextWnd)
     hTextWnd->SetWindowText(m_strText);

  CenterWindow(this->GetParent()->GetParent());  // GetDesktopWindow()
  BringWindowToTop();
  ShowWindow(SW_SHOW);
  RedrawWindow();

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

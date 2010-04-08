/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : waitdlgprogress.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : Joseph Cuccia 
**    Purpose  : Displays progress bar during creation of log files 
**               See the CONCEPT.DOC for more detail
**
** History:
**
** 02-Dec-1998 (cucjo01)
**    created
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
**/


#include "stdafx.h"
#include "vcbf.h"
#include "WaitDlgProgress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaitDlgProgress dialog

CWaitDlgProgress::CWaitDlgProgress(CString strText, UINT uiAVI_Resource, UINT uiDialogID /* = IDD_WAIT_DIALOG_PROGRESS */, CWnd* pParent /* = NULL */)
	: CDialog(CWaitDlgProgress::IDD, pParent)
{
    m_uiAVI_Resource = uiAVI_Resource;
    m_strText = strText;

    Create(uiDialogID);
}

void CWaitDlgProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaitDlgProgress)
	DDX_Control(pDX, IDC_AVI, m_Avi);
	DDX_Control(pDX, IDC_PROGRESS1, m_ProgressBar);
	DDX_Text(pDX, IDC_TEXT, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaitDlgProgress, CDialog)
	//{{AFX_MSG_MAP(CWaitDlgProgress)
	ON_WM_CANCELMODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlgProgress message handlers

BOOL CWaitDlgProgress::OnInitDialog() 
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

  m_ProgressBar.SetStep(1);
  m_ProgressBar.SetRange(0,63);
  
  CenterWindow(this->GetParent()->GetParent());  // GetDesktopWindow()
  BringWindowToTop();
  ShowWindow(SW_SHOW);
  RedrawWindow();
	
  return TRUE;  // return TRUE unless you set the focus to a control
}

void CWaitDlgProgress::StepProgressBar() 
{ 
   if(::IsWindow(m_ProgressBar.m_hWnd))
      m_ProgressBar.StepIt();
}

void CWaitDlgProgress::SetText(LPCTSTR szText)
{ 
  CWnd *hTextWnd = GetDlgItem(IDC_TEXT);
  if ((hTextWnd) && (szText))
     hTextWnd->SetWindowText(szText);
}

/******************************************************************************
**
** Copyright (c) 1998 Ingres Corporation
**
******************************************************************************/

/******************************************************************************
**
** WaitDlg.cpp : implementation file
**
** History:
**	25-jan-99 (cucjo01) (created.)
**	    Added multi-threaded wait dialog with animation
**	30-mar-1999 (somsa01)
**	    Taken from Enterprise Installer and personalized.
**	13-apr-1999 (somsa01)
**	    Call LoadString() with the Instance handle.
**	27-apr-1999 (somsa01)
**	    Use CStrings.
**
******************************************************************************/

#include "stdafx.h"
#include "iilink.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HWND MainWnd;

/*
** CWaitDlg dialog
*/
CWaitDlg::CWaitDlg(CWnd* pParent)
: CDialog(CWaitDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWaitDlg)
	// NOTE: the ClassWizard will add member initialization here
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
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CWaitDlg message handlers
*/
BOOL
CWaitDlg::OnInitDialog() 
{
    UINT	m_uiAVI_Resource = IDR_CLOCK_AVI;
    CString	Message;

    CDialog::OnInitDialog();

    Message.LoadString(IDS_WAIT_HEADING);
    SetDlgItemText(IDC_WAIT_HEADING, Message);
	
    CWnd *hAniWnd = GetDlgItem(IDC_AVI);
    if (hAniWnd)
    {
	hAniWnd->SendMessage(ACM_OPEN, 0,
			(LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_uiAVI_Resource)));
	hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1),
			(LPARAM)MAKELONG(0, 0xFFFF));
    }

    CenterWindow(GetDesktopWindow());
    BringWindowToTop();
    ShowWindow(SW_SHOW);
    RedrawWindow();

    MainWnd = m_hWnd;
	
    return TRUE;
}

void
CWaitDlg::OnClose() 
{
    MainWnd = NULL;
    CDialog::OnClose();
}

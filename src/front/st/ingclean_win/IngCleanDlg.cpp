/*
**  Copyright (c) 2003 Ingres Corporation
*/

/*
**  Name: IngCleanDlg.cpp : implementation file
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/

#include "stdafx.h"
#include "IngClean.h"
#include "IngCleanDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIngCleanDlg dialog

CIngCleanDlg::CIngCleanDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIngCleanDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CIngCleanDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CIngCleanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIngCleanDlg)
	DDX_Control(pDX, IDC_TEXT, m_text);
	DDX_Control(pDX, IDC_OUTPUT, m_output);
	DDX_Control(pDX, IDC_COPYRIGHT, m_copyright);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIngCleanDlg, CDialog)
	//{{AFX_MSG_MAP(CIngCleanDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDUNINSTALL, OnUninstall)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIngCleanDlg message handlers

BOOL CIngCleanDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	CString s;
	s.Format(IDS_TEXT, theUninst.m_id);
	m_text.SetWindowText(s);

	CTime t = CTime::GetCurrentTime();
	s.Format(IDS_COPYRIGHT, t.GetYear());
	m_copyright.SetWindowText(s);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIngCleanDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CIngCleanDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CIngCleanDlg::OnTimer(UINT nIDEvent) 
{
	if (!theUninst.IsRunning())
	{
		KillTimer(1);
		Message(theUninst.m_succeeded ? IDS_SUCCEEDED : IDS_NOTSUCCESSFULL, theUninst.m_id);
		//Message(IDS_RESET_II_SYSTEM);
		EndDialog(TRUE);
	}
}

void CIngCleanDlg::OnCancel() 
{
    if (AskUserYN(IDS_AREYOUSURE))
		CDialog::OnCancel();
}

void CIngCleanDlg::OnUninstall() 
{
	// TODO: Add your control notification handler code here
	theUninst.Uninstall(&m_output);
	SetTimer(1,200,0);
}

void CIngCleanDlg::OnClose() 
{
	return;
}

BOOL CIngCleanDlg::PreTranslateMessage(MSG* pMsg) 
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

/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dlgmain.cpp , Implementation File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01) 
**    Purpose  : main dialog 
**
** History:
**
** 14-Dec-1998 (uk$so01)
**    created.
** 17-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client".
*/

#include "stdafx.h"
#include "ivm.h"
#include "dlgmain.h"
#include "ivmdml.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain dialog


CuDlgMain::CuDlgMain(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgMain::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgMain)
	m_strInstallation = _T("Installation");
	m_strHost = _T("Host");
	m_strIISystem = _T("IISYSTEM");
	//}}AFX_DATA_INIT
	m_pManagerFrame = NULL;
	m_nStartCount = 0;
	m_bAnimationStarted = FALSE;
	IVM_GetHostInfo (m_strInstallation, m_strHost, m_strIISystem);
}


void CuDlgMain::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgMain)
	DDX_Control(pDX, IDC_ANIMATE1, m_cAnimate1);
	DDX_Control(pDX, IDC_STATIC3, m_cStaticIISystem);
	DDX_Control(pDX, IDC_STATIC_FRAME, m_cStaticFrame);
	DDX_Text(pDX, IDC_STATIC1, m_strInstallation);
	DDX_Text(pDX, IDC_STATIC2, m_strHost);
	DDX_Text(pDX, IDC_STATIC3, m_strIISystem);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgMain, CDialog)
	//{{AFX_MSG_MAP(CuDlgMain)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_START_STOP, OnStartStop)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain message handlers

void CuDlgMain::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgMain::OnInitDialog() 
{
	CDialog::OnInitDialog();
	try
	{
		CvManagerViewRight* pView = (CvManagerViewRight*)GetParent();
		CdMainDoc* pDoc = (CdMainDoc*)pView->GetDocument();
		CRect r;
		m_cStaticFrame.GetWindowRect (r);
		ScreenToClient (r);
		ASSERT (pDoc);
		m_pManagerFrame = new CfManagerFrame(pDoc);
		m_pManagerFrame->Create (
			NULL,
			NULL,
			WS_CHILD,
			r,
			this);
		m_pManagerFrame->ShowWindow (SW_SHOW);
		m_pManagerFrame->InitialUpdateFrame (NULL, TRUE);
		CvManagerViewLeft* pLeftView = m_pManagerFrame->GetLeftView();
		if (pLeftView)
			pLeftView->InitializeData();
		if (IsWindow(m_cAnimate1.m_hWnd))
		{
			m_cAnimate1.Open(IDR_ANCLOCK);
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuDlgMain::OnInitDialog(), Cannot create panes"));
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgMain::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	//
	// Resize the frame Window.
	if (m_pManagerFrame && IsWindow(m_pManagerFrame->m_hWnd)) 
	{
		CRect r, rDlg;
		GetClientRect (rDlg);

		m_cStaticFrame.GetWindowRect (r);
		ScreenToClient (r);
		r.bottom = rDlg.bottom;
		r.left   = rDlg.left;
		r.right  = rDlg.right;
		m_pManagerFrame->MoveWindow(r);
		if (m_cAnimate1.IsWindowVisible())
		{
			m_cAnimate1.GetWindowRect (r);
			ScreenToClient (r);
			int w = r.Width();
			r.right = rDlg.right-2;
			r.left  = r.right - w;
			m_cAnimate1.MoveWindow (r);
			CRect r1;
			m_cStaticIISystem.GetWindowRect (r1);
			ScreenToClient (r1);
			r1.right  = r.left - 2;
			m_cStaticIISystem.MoveWindow(r1);
		}
		else
		{
			m_cStaticIISystem.GetWindowRect (r);
			ScreenToClient (r);
			r.right  = rDlg.right - 2;
			m_cStaticIISystem.MoveWindow(r);
		}
	}
}

void CuDlgMain::StartAnimation()
{
	if (!IsWindow (m_cAnimate1.m_hWnd))
		return;
	if (m_bAnimationStarted)
		return;
	CRect r1, r2, rDlg;
	GetClientRect (rDlg);

	m_cAnimate1.GetWindowRect (r1);
	ScreenToClient (r1);
	int w = r1.Width();
	r1.right = rDlg.right-2;
	r1.left  = r1.right - w;

	m_cStaticIISystem.GetWindowRect (r2);
	ScreenToClient (r2);
	r2.right  = r1.left - 2;
	m_cStaticIISystem.MoveWindow(r2);
	
	m_cAnimate1.Play(0, 0xFFFF, (UINT)-1);
	m_cAnimate1.MoveWindow (r1);
	m_cAnimate1.ShowWindow (SW_SHOW);
	m_bAnimationStarted = TRUE;
}

void CuDlgMain::StopAnimation()
{
	if (!IsWindow (m_cAnimate1.m_hWnd))
		return;
	
	CRect r, rDlg;
	GetClientRect (rDlg);
	m_cAnimate1.ShowWindow (SW_HIDE);
	m_cAnimate1.Stop();

	m_cStaticIISystem.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right - 2;
	m_cStaticIISystem.MoveWindow(r);
	m_bAnimationStarted = FALSE;
}

LONG CuDlgMain::OnStartStop (WPARAM wParam, LPARAM lParam)
{
	if (!theApp.IsShowStartStopAnimation())
		return 0;
	int id = (int)wParam;
	if (id == ID_START || id == ID_START_CLIENT)
		StartAnimation();
	else
		StopAnimation();

	return 0;
}

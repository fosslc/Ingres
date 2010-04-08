/*
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : configdlg.cpp, Implementation file
**
**
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**
**
**    Purpose  : Special Page of CMainTabDlg (Configure)
**               See the CONCEPT.DOC for more detail
**
**  History:
**	01-nov-2001 (somsa01)
**	    CLeaned up 64-bit compiler warnings / errors.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "cconfdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog


CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
	m_pConfDlgFrame = (CConfigFrame *)0;
	//{{AFX_DATA_INIT(CConfigDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_COMPONENT_EXIT,  OnComponentExit)
	ON_MESSAGE (WM_COMPONENT_ENTER, OnComponentEnter)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers

void CConfigDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	//
	// Resize the frame Window.

	if (m_pConfDlgFrame && IsWindow(m_pConfDlgFrame->m_hWnd)) 
	{
		CRect r;
		GetClientRect(r);
		r.top    += 10;
		r.bottom -= 10;
		r.left   += 10;
		r.right  -= 10;
		m_pConfDlgFrame->MoveWindow(r);
	}
}

BOOL CConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CRect    r;
	GetClientRect (r);
	 	
	m_pConfDlgFrame	= new CConfigFrame();
	m_pConfDlgFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this);
	m_pConfDlgFrame->ShowWindow (SW_SHOW);
	m_pConfDlgFrame->InitialUpdateFrame (NULL, TRUE);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}


LONG CConfigDlg::OnComponentExit (UINT wParam, LONG lParam)
{
	if (m_pConfDlgFrame)
		(m_pConfDlgFrame->GetCConfLeftDlg())->SendMessage (WM_COMPONENT_EXIT, 0, 0);
	return 0;
}

LONG CConfigDlg::OnComponentEnter (UINT wParam, LONG lParam)
{
	if (m_pConfDlgFrame)
		(m_pConfDlgFrame->GetCConfLeftDlg())->SendMessage (WM_COMPONENT_ENTER, 0, 0);
	return 0;
}

LONG CConfigDlg::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	if (m_pConfDlgFrame)
		(m_pConfDlgFrame->GetCConfLeftDlg())->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
	return 0;
}

LONG CConfigDlg::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	if (m_pConfDlgFrame)
	{
		(m_pConfDlgFrame->GetCConfLeftDlg())->SendMessage  (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);
		(m_pConfDlgFrame->GetCConfRightDlg())->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);
	}
	return 0;
}

UINT CConfigDlg::GetHelpID()
{
	ASSERT (m_pConfDlgFrame);
	return m_pConfDlgFrame? m_pConfDlgFrame->GetHelpID(): 0;
}

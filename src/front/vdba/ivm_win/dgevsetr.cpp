/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dgevsetr.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog of the right pane of Event Setting Frame
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**
**/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgevsetr.h"
#include "fevsetti.h"
#include "ivmsgdml.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgEventSettingRight dialog


CuDlgEventSettingRight::CuDlgEventSettingRight(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgEventSettingRight::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgEventSettingRight)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgEventSettingRight::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgEventSettingRight)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckShowStateBranches);
	DDX_Control(pDX, IDC_TREE1, m_cTree1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgEventSettingRight, CDialog)
	//{{AFX_MSG_MAP(CuDlgEventSettingRight)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckShowStateBranches)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnDisplayTree)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgEventSettingRight message handlers

void CuDlgEventSettingRight::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgEventSettingRight::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ImageList.Create (IDB_MESSAGE_CATEGORY_SETTING, 16, 0, RGB(255,0,255));
	m_cTree1.SetImageList (&m_ImageList, TVSIL_NORMAL);

	m_cCheckShowStateBranches.SetCheck (1);
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();

	CaTreeMessageCategory& treeCategoryView = pFrame->GetTreeCategoryView();
	treeCategoryView.Display (&m_cTree1, NULL, SHOW_STATE_BRANCH);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgEventSettingRight::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cTree1.m_hWnd))
		return;
	CRect r, rDlg;
	GetClientRect (rDlg);
	m_cTree1.GetWindowRect (r);
	ScreenToClient (r);
	r.left  = rDlg.left;
	r.right = rDlg.right - 1;
	r.bottom= rDlg.bottom- 1;
	m_cTree1.MoveWindow (r);
}

void CuDlgEventSettingRight::OnCheckShowStateBranches() 
{
	BOOL bShow = (BOOL)m_cCheckShowStateBranches.GetCheck();
	UINT nMode = bShow? SHOW_STATE_BRANCH: 0;
	nMode |= SHOW_CHANGED;

	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	CaTreeMessageCategory& treeCategoryView = pFrame->GetTreeCategoryView();
	treeCategoryView.Display (&m_cTree1, NULL, nMode);
}



//
// If w = 1 then do not notify the left tree:
LONG CuDlgEventSettingRight::OnDisplayTree (WPARAM w, LPARAM l)
{
	BOOL bShow = (BOOL)m_cCheckShowStateBranches.GetCheck();
	UINT nMode = bShow? SHOW_STATE_BRANCH: 0;
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	CaTreeMessageCategory& treeCategoryView = pFrame->GetTreeCategoryView();

	treeCategoryView.Display (&m_cTree1, NULL, nMode);
	treeCategoryView.SortItems (&m_cTree1);
	m_cTree1.Invalidate();
	//
	// Notify the left tree to re-display:
	if ((int)w == 1)
		return 0;
	CvEventSettingLeft* pLeftView = pFrame->GetLeftView();
	if (pLeftView)
	{
		CuDlgEventSettingLeft* pDlg = pLeftView->GetDialog();
		if (pDlg)
			pDlg->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)1, l);
	}
	
	return 0;
}

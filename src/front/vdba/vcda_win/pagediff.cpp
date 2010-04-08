/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   
*/

/*
**    Source   : pagediff.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of tab control to display the list of differences
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
*/


#include "stdafx.h"
#include "vcda.h"
#include "pagediff.h"
#include "vcdadml.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CuDlgPageDifference::Init()
{
	m_bVirtualNodePage = FALSE;
}


CuDlgPageDifference::CuDlgPageDifference(BOOL bVirtualNodePage, CWnd* pParent)
	: CDialog(CuDlgPageDifference::IDD, pParent)
{
	Init();
	m_bVirtualNodePage = TRUE;
}


CuDlgPageDifference::CuDlgPageDifference(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgPageDifference::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgPageDifference)
	//}}AFX_DATA_INIT
	Init();
}


void CuDlgPageDifference::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPageDifference)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPageDifference, CDialog)
	//{{AFX_MSG_MAP(CuDlgPageDifference)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPageDifference message handlers

void CuDlgPageDifference::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgPageDifference::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

long CuDlgPageDifference::OnUpdateData(WPARAM w, LPARAM l)
{
	if (!l)
	{
		m_cListCtrl.DeleteAllItems();
	}

	return 0;
}

BOOL CuDlgPageDifference::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));

	LSCTRLHEADERPARAMS2 lsp[3] =
	{
		{IDS_TAB_PARAMETER, 250,  LVCFMT_LEFT, FALSE},
		{IDS_TAB_SNAPSHOT1, 200,  LVCFMT_LEFT, FALSE},
		{IDS_TAB_SNAPSHOT2, 200,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, 3);
	m_ImageList.Create (IDB_DIFFERENCE, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPageDifference::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_cListCtrl.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_cListCtrl.MoveWindow(r);
	}
}

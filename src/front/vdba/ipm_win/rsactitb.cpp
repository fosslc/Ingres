/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/


/*
**  Project  : Visual DBA
**  Source   : rsactitb.cpp, Implementation file
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : Page of a static item Replication.(Sub page of Activity: Per Table)
**             Outgoing, Incoming, Total 
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "rsactitb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationStaticPageActivityPerTable::CuDlgReplicationStaticPageActivityPerTable(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationStaticPageActivityPerTable::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageActivityPerTable)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nMode = 0;
}

CuDlgReplicationStaticPageActivityPerTable::CuDlgReplicationStaticPageActivityPerTable(int nMode, CWnd* pParent)
	: CDialog(CuDlgReplicationStaticPageActivityPerTable::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageActivityPerTable)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nMode = nMode;
}

void CuDlgReplicationStaticPageActivityPerTable::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationStaticPageActivityPerTable)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationStaticPageActivityPerTable, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationStaticPageActivityPerTable)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

void CuDlgReplicationStaticPageActivityPerTable::GetAllColumnWidth (CArray < int, int >& cxArray)
{
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_WIDTH;

	const int LAYOUT_NUMBER = 6;
	int i, hWidth   [LAYOUT_NUMBER] = {120, 80, 80, 80, 80, 100};
	for (i=0; i<LAYOUT_NUMBER; i++)
	{
		if (m_cListCtrl.GetColumn(i, &lvcolumn))
			cxArray.InsertAt (i, lvcolumn.cx);
		else
			cxArray.InsertAt (i, hWidth[i]);
	}
}

void CuDlgReplicationStaticPageActivityPerTable::SetAllColumnWidth (CArray < int, int >& cxArray)
{
	const int LAYOUT_NUMBER = 6;
	for (int i=0; i<LAYOUT_NUMBER; i++)
		m_cListCtrl.SetColumnWidth(i, cxArray.GetAt(i));
}


CSize CuDlgReplicationStaticPageActivityPerTable::GetScrollPosListCtrl ()
{
	return CSize (m_cListCtrl.GetScrollPos (SB_HORZ), m_cListCtrl.GetScrollPos (SB_VERT));
}

void CuDlgReplicationStaticPageActivityPerTable::SetScrollPosListCtrl (CSize& size)
{
	m_cListCtrl.SetScrollPos (SB_HORZ, size.cx);
	m_cListCtrl.SetScrollPos (SB_VERT, size.cx);
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationStaticPageActivityPerTable message handlers

void CuDlgReplicationStaticPageActivityPerTable::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}

BOOL CuDlgReplicationStaticPageActivityPerTable::OnInitDialog() 
{
	ASSERT (m_nMode != 0);
	if (m_nMode == 0)
		return FALSE;
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	const int LAYOUT_NUMBER = 7;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_DATABASE,    120, LVCFMT_LEFT,  FALSE},
		{IDS_TC_TABLE,        80, LVCFMT_LEFT,  FALSE},
		{IDS_TC_INSERTS,      80, LVCFMT_RIGHT, FALSE},
		{IDS_TC_UPDATES,      80, LVCFMT_RIGHT, FALSE},
		{IDS_TC_DELETES,      80, LVCFMT_RIGHT, FALSE},
		{IDS_TC_TOTAL,        80, LVCFMT_RIGHT, FALSE},
		{IDS_TC_TRANSACTIONS,100, LVCFMT_RIGHT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationStaticPageActivityPerTable::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect rDlg;
	GetClientRect (rDlg);
	m_cListCtrl.MoveWindow (rDlg);
}

LONG CuDlgReplicationStaticPageActivityPerTable::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	return 0L;
}

LONG CuDlgReplicationStaticPageActivityPerTable::OnLoad (WPARAM wParam, LPARAM lParam)
{
	return 0L;
}

LONG CuDlgReplicationStaticPageActivityPerTable::OnGetData (WPARAM wParam, LPARAM lParam)
{
	return (LRESULT)NULL;
}

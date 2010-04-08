/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dglishdr.cpp, implementation file
**    Project  : IJA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless dialog that contains the list control 
**               with header (generic)
**               The caller must initialize the following member
**               before creating the dialog:
**               + m_nHeaderCount: number of header in the list control:
**               + m_pArrayHeader: array of headers
**               + m_lpfnDisplay : the funtion that display the item in the list control.
**               + m_pWndMessageHandler: to which the list control will send message to.
**
** History :
**
** 16-May-2000 (uk$so01)
**    created
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dglishdr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgListHeader::CuDlgListHeader(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgListHeader::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgListHeader)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nHeaderCount = 0;
	m_pArrayHeader = NULL;
	m_lpfnDisplay  = NULL;
	m_pWndMessageHandler = NULL;
}


void CuDlgListHeader::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgListHeader)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgListHeader, CDialog)
	//{{AFX_MSG_MAP(CuDlgListHeader)
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnClickList1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgListHeader message handlers

BOOL CuDlgListHeader::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cImageList.Create (1, 16, TRUE, 1, 0);
	
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_cListCtrl.SetLineSeperator(FALSE);
	m_cListCtrl.SetImageList (&m_cImageList, LVSIL_SMALL);

	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	CString strHeader;

	for (int i=0; i<m_nHeaderCount; i++)
	{
		strHeader.LoadString (m_pArrayHeader[i].m_nIDS);
		lvcolumn.fmt      = m_pArrayHeader[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = m_pArrayHeader[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgListHeader::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgListHeader::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

void CuDlgListHeader::AddItem (LPARAM lItem)
{
	int nCount = m_cListCtrl.GetItemCount();
	int nIdx = m_cListCtrl.InsertItem (nCount, _T(""));
	if (nIdx != -1)
	{
		m_cListCtrl.SetItemData (nIdx, (DWORD)lItem);
		if (m_lpfnDisplay)
			m_lpfnDisplay (&m_cListCtrl, lItem, nIdx);
	}
}



void CuDlgListHeader::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (m_pWndMessageHandler && IsWindow (m_pWndMessageHandler->m_hWnd))
	{
		if (pNMListView->iItem >= 0 && (m_cListCtrl.GetItemState (pNMListView->iItem, LVIS_SELECTED)&LVIS_SELECTED))
		{
			DWORD dwItemData = m_cListCtrl.GetItemData (pNMListView->iItem );
			m_pWndMessageHandler->SendMessage (WM_LISTCTRLITEM_PROPERTY, 1, (LPARAM)dwItemData);
		}
	}
	*pResult = 0;
}

void CuDlgListHeader::OnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (m_pWndMessageHandler && IsWindow (m_pWndMessageHandler->m_hWnd))
	{
		if (pNMListView->iItem >= 0 && (m_cListCtrl.GetItemState (pNMListView->iItem, LVIS_SELECTED)&LVIS_SELECTED))
		{
			DWORD dwItemData = m_cListCtrl.GetItemData (pNMListView->iItem );
			m_pWndMessageHandler->SendMessage (WM_LISTCTRLITEM_PROPERTY, 0, (LPARAM)dwItemData);
		}
	}
	
	*pResult = 0;
}

/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : netrgdlg.cpp : implementation file
**    Project  : Configuration Manager 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog, Page (Registry protocol) of Net
**
** History:
**
** 20-jun-2003 (uk$so01)
**    created.
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
*/

#include "stdafx.h"
#include "vcbf.h"
#include "netrgdlg.h"
#include "crightdg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CuDlgNetRegistry::CuDlgNetRegistry(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgNetRegistry::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgNetRegistry)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgNetRegistry::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgNetRegistry)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgNetRegistry, CDialog)
	//{{AFX_MSG_MAP(CuDlgNetRegistry)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnEditListenAddress)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgNetRegistry message handlers

void CuDlgNetRegistry::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgNetRegistry::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

BOOL CuDlgNetRegistry::OnInitDialog() 
{
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.SetCheckImageList(&m_ImageListCheck);

	const int nColumn = 3;
	LVCOLUMN lvcolumn;
	UINT strHeaderID[nColumn] = { IDS_COL_PROTOCOL, IDS_COL_STATUS, IDS_COL_LISTEN_ADDRESS};
	int i, hWidth   [nColumn] = {100 , 100, 100};
	int fmt [nColumn] = {LVCFMT_LEFT , LVCFMT_CENTER, LVCFMT_LEFT};
	//
	// Set the number of columns: nColumn
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<nColumn; i++)
	{
		lvcolumn.fmt = fmt[i];
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i==1)
			m_cListCtrl.InsertColumn (i, &lvcolumn, TRUE); // TRUE---> uses checkbox
		else
			m_cListCtrl.InsertColumn (i, &lvcolumn); 
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgNetRegistry::OnDestroy() 
{
	CDialog::OnDestroy();
	ResetList();
}

void CuDlgNetRegistry::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Restore)
	BOOL bEnable = (m_cListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton1 && pButton2)
	{
		pButton1->EnableWindow (bEnable);
		pButton2->EnableWindow (bEnable);
	}
	*pResult = 0;
}

LONG CuDlgNetRegistry::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_LISTEN_ADDRESS);
	pButton1->SetWindowText (csButtonTitle);

	pButton1->ShowWindow(SW_SHOW);
	pButton1->EnableWindow(FALSE);
	pButton2->ShowWindow(SW_HIDE);

	//
	// Fill control(s)
	ResetList();

	VCBFll_netprots_init(NET_REGISTRY);
	NETPROTLINEINFO protocol;
	BOOL bFirstPass = TRUE;
	while (1) {
		BOOL bOk;
		if (bFirstPass) {
			bOk = VCBFllGetFirstNetProtLine(&protocol);
			bFirstPass = FALSE;
		}
		else
			bOk = VCBFllGetNextNetProtLine(&protocol);
		if (!bOk)
			break;    // last item encountered
		if (!AddProtocol(&protocol)) {
			AfxMessageBox(IDS_ERR_CREATE_PROTO_LIST, MB_ICONSTOP | MB_OK);
			return FALSE;
		}
	}

	return 0L;
}

LONG CuDlgNetRegistry::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_cListCtrl.HideProperty();
	return 0L;
}


void CuDlgNetRegistry::ResetList()
{
	int nCount;
	
	nCount = m_cListCtrl.GetItemCount();
	for (int i=0; i<nCount; i++)
	{
		LPNETPROTLINEINFO ptr = (LPNETPROTLINEINFO)m_cListCtrl.GetItemData (i);
		ASSERT (ptr);
		if (ptr)
			delete ptr;
	}
	m_cListCtrl.DeleteAllItems();
}

BOOL CuDlgNetRegistry::AddProtocol(void* lpProtocol)
{
	int nCount, index;
	LPNETPROTLINEINFO pObj = (LPNETPROTLINEINFO)lpProtocol;

	// fill columns
	nCount = m_cListCtrl.GetItemCount ();
	index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pObj->szprotocol);
	if (_tcsicmp((LPCTSTR)pObj->szstatus, _T("ON")) == 0) // On/Off
		m_cListCtrl.SetCheck (index, 1, TRUE);
	else
		m_cListCtrl.SetCheck (index, 1, FALSE);
	m_cListCtrl.SetItemText (index, 2, (LPCTSTR)pObj->szlisten);

	// allocate and set data
	LPNETPROTLINEINFO lpDupProtocol = new NETPROTLINEINFO;
	memcpy(lpDupProtocol, pObj, sizeof(NETPROTLINEINFO));
	lpDupProtocol->nProtocolType = NET_REGISTRY;
	BOOL bSuccess = m_cListCtrl.SetItemData (index, (DWORD_PTR)lpDupProtocol);

	return bSuccess;
}

LONG CuDlgNetRegistry::OnEditListenAddress (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgNetRegistry::OnEditListenAddress...\n");

	// listen address is in column 2
	int iColumn = 2;

	int index = m_cListCtrl.GetCurSel();
	if (index < 0)
		return 0L;

	CRect rItem, rCell;
	m_cListCtrl.GetItemRect (index, rItem, LVIR_BOUNDS);
	BOOL bOk = m_cListCtrl.GetCellRect (rItem, iColumn, rCell);
	if (bOk) 
	{
		m_cListCtrl.HideProperty();
		m_cListCtrl.SetEditText (index, iColumn, rCell);
	}

	return 0L;
}

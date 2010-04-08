/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : netprdlg.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**               Emmanuel Blattes (Custom implementations)
**    Purpose  : Modeless Dialog, Page (Parameter) of NET Server
**               See the CONCEPT.DOC for more detail
**
**  History:
**	xx-Sep-1997 (uk$so01)
**	    created
**	06-Jun-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 20-jun-2003 (uk$so01)
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
#include "netprdlg.h"
#include "wmusrmsg.h"
#include "crightdg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgNetProtocol dialog
CuDlgNetProtocol::CuDlgNetProtocol(CWnd* pParent)
	: CDialog(CuDlgNetProtocol::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgNetProtocol)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgNetProtocol::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgNetProtocol)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgNetProtocol, CDialog)
	//{{AFX_MSG_MAP(CuDlgNetProtocol)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnEditListenAddress)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuDlgNetProtocol message handlers


void CuDlgNetProtocol::OnDestroy() 
{
	CDialog::OnDestroy();
	
  // custom code
  ResetList();
}

BOOL CuDlgNetProtocol::OnInitDialog() 
{
	const int LAYOUT_NUMBER = 3;
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetCheckImageList(&m_ImageListCheck);

	LV_COLUMN lvcolumn;
	UINT strHeaderID[LAYOUT_NUMBER] = { IDS_COL_PROTOCOL, IDS_COL_STATUS, IDS_COL_LISTEN_ADDRESS};
	int i, hWidth   [LAYOUT_NUMBER] = {100 , 100, 100};
	int fmt [LAYOUT_NUMBER] = {LVCFMT_LEFT , LVCFMT_CENTER, LVCFMT_LEFT};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER; i++)
	{
		lvcolumn.fmt = fmt[i];
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
	if (i==1)
		m_ListCtrl.InsertColumn (i, &lvcolumn, TRUE); // TRUE---> uses checkbox
	else
		m_ListCtrl.InsertColumn (i, &lvcolumn); 
	}
	return TRUE;
}

LONG CuDlgNetProtocol::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);

	// 29 Sept. 97: No button for checkbox
		/*
	Old code:
	pButton1->SetWindowText ("Status");
		pButton2->SetWindowText ("Listen Address");
	*/
	pButton2->ShowWindow(SW_HIDE);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_LISTEN_ADDRESS);
	pButton1->SetWindowText (csButtonTitle);

	pButton1->ShowWindow(SW_SHOW);
	
	//
	// Fill control(s)
	//
	
	ResetList();
	
	VCBFll_netprots_init();
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

LONG CuDlgNetProtocol::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_ListCtrl.HideProperty();
	return 0L;
}


BOOL CuDlgNetProtocol::AddProtocol(LPNETPROTLINEINFO lpProtocol)
{
	int nCount, index;

  // fill columns
	nCount = m_ListCtrl.GetItemCount ();
	index  = m_ListCtrl.InsertItem (nCount, (LPCTSTR)lpProtocol->szprotocol);
  if (_tcscmp((LPCTSTR)lpProtocol->szstatus, _T("ON")) == 0) // On/Off
  	m_ListCtrl.SetCheck (index, 1, TRUE);
  else
  	m_ListCtrl.SetCheck (index, 1, FALSE);
  m_ListCtrl.SetItemText (index, 2, (LPCTSTR)lpProtocol->szlisten);

  // allocate and set data
  LPNETPROTLINEINFO lpDupProtocol = new NETPROTLINEINFO;
  memcpy(lpDupProtocol, lpProtocol, sizeof(NETPROTLINEINFO));
  lpDupProtocol->nProtocolType = NET_PROTOCOL;
  BOOL bSuccess = m_ListCtrl.SetItemData (index, (DWORD_PTR)lpDupProtocol);

  return bSuccess;
}

void CuDlgNetProtocol::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgNetProtocol::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}

void CuDlgNetProtocol::ResetList()
{
	int nCount;
	
	nCount = m_ListCtrl.GetItemCount();
	for (int i=0; i<nCount; i++)
	{
		LPNETPROTLINEINFO ptr = (LPNETPROTLINEINFO)m_ListCtrl.GetItemData (i);
		ASSERT (ptr);
		if (ptr)
			delete ptr;
	}
	m_ListCtrl.DeleteAllItems();
}

LONG CuDlgNetProtocol::OnEditListenAddress (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgNetProtocol::OnEditListenAddress...\n");

  // listen address is in column 2
  int iColumn = 2;

	int index = m_ListCtrl.GetCurSel();
	if (index < 0)
		return 0L;

	CRect rItem, rCell;
	m_ListCtrl.GetItemRect (index, rItem, LVIR_BOUNDS);
  BOOL bOk = m_ListCtrl.GetCellRect (rItem, iColumn, rCell);
  if (bOk) {
  	m_ListCtrl.HideProperty();
    m_ListCtrl.SetEditText (index, iColumn, rCell);
  }

	return 0L;
}

void CuDlgNetProtocol::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Restore)
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
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

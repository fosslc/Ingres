/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipicht.cpp, Implementation file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Ice Http Server Connection Page 
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipicht.h"
#include "ipmicdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgIpmIcePageHttpServerConnection::CuDlgIpmIcePageHttpServerConnection(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmIcePageHttpServerConnection::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmIcePageHttpServerConnection)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmIcePageHttpServerConnection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIcePageHttpServerConnection)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIcePageHttpServerConnection, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmIcePageHttpServerConnection)
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkMfcList1)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST1, OnDeleteitemMfcList1)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIcePageHttpServerConnection message handlers

void CuDlgIpmIcePageHttpServerConnection::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmIcePageHttpServerConnection::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create(IDB_TM_ICE_HTTP_CONNECTION, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[4] =
	{
		{IDS_TC_REAL_USER,    100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_ID,            40,  LVCFMT_RIGHT, FALSE},
		{IDS_TC_STATE,        200,  LVCFMT_LEFT, FALSE},
		{IDS_TC_WAIT_REASON,  100,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, 4);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmIcePageHttpServerConnection::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmIcePageHttpServerConnection::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	SESSIONDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	default:
		return 0L;
	}

	try
	{
		SESSIONDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_SESSION, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (SESSIONDATAMIN*)listInfoStruct.RemoveHead();
				AddItem (_T(""), pData);
			}
		}
		DisplayItems();
		return 0L;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	while (!listInfoStruct.IsEmpty())
		delete (SESSIONDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmIcePageHttpServerConnection::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIcePageHttpServerConnection")) == 0);
	CTypedPtrList<CPtrList, SESSIONDATAMIN*>* pListTuple;
	pListTuple = (CTypedPtrList<CPtrList, SESSIONDATAMIN*>*)lParam;

	SESSIONDATAMIN* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (SESSIONDATAMIN*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			if (pObj)
				AddItem (_T(""), pObj);
		}
		DisplayItems();
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmIcePageHttpServerConnection::OnGetData (WPARAM wParam, LPARAM lParam)
{
	SESSIONDATAMIN* p = NULL;
	CaIpmPropertyDataIcePageHttpServerConnection* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIcePageHttpServerConnection ();
		CTypedPtrList<CPtrList, SESSIONDATAMIN*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			SESSIONDATAMIN* pItem = (SESSIONDATAMIN*)m_cListCtrl.GetItemData(i);
			if (!pItem)
				continue;
			p = new SESSIONDATAMIN;
			memcpy (p, pItem, sizeof(SESSIONDATAMIN));
			listTuple.AddTail (p);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIcePageHttpServerConnection::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}

void CuDlgIpmIcePageHttpServerConnection::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	SESSIONDATAMIN* p = (SESSIONDATAMIN*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (p != NULL)
		delete p;
	*pResult = 0;
}

void CuDlgIpmIcePageHttpServerConnection::AddItem (LPCTSTR lpszItem, SESSIONDATAMIN* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgIpmIcePageHttpServerConnection::DisplayItems()
{
	CString strItem;
	TCHAR tchszBuffer[32];
	SESSIONDATAMIN* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (SESSIONDATAMIN*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, (LPCTSTR)p->real_user);
		m_cListCtrl.SetItemText (i, 1, (LPCTSTR)FormatHexa64 (p->session_id, tchszBuffer));
		m_cListCtrl.SetItemText (i, 2, (LPCTSTR)p->session_state);
		m_cListCtrl.SetItemText (i, 3, (LPCTSTR)p->session_wait_reason);
	}
	
}


void CuDlgIpmIcePageHttpServerConnection::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

void CuDlgIpmIcePageHttpServerConnection::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

long CuDlgIpmIcePageHttpServerConnection::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}

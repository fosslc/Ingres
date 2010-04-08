/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipictr.cpp, Implementation file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Ice Http Server Connection Page 
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgipictr.h"
#include "ipmicdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgIpmIcePageTransaction::CuDlgIpmIcePageTransaction(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmIcePageTransaction::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmIcePageTransaction)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmIcePageTransaction::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIcePageTransaction)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIcePageTransaction, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmIcePageTransaction)
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
// CuDlgIpmIcePageTransaction message handlers

void CuDlgIpmIcePageTransaction::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmIcePageTransaction::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create(IDB_TM_ICE_ALLTRANSACTION, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[4] =
	{
		{IDS_TC_KEY,         100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_NAME,        100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_USR_SESS,    100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_CONNECT_KEY, 100,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, 4);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmIcePageTransaction::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmIcePageTransaction::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	ICE_TRANSACTIONDATAMIN* pData;
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
		ICE_TRANSACTIONDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_ICE_TRANSACTION, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (ICE_TRANSACTIONDATAMIN*)listInfoStruct.RemoveHead();
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
		delete (ICE_TRANSACTIONDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmIcePageTransaction::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIcePageTransaction")) == 0);
	CTypedPtrList<CPtrList, ICE_TRANSACTIONDATAMIN*>* pListTuple;
	pListTuple = (CTypedPtrList<CPtrList, ICE_TRANSACTIONDATAMIN*>*)lParam;

	ICE_TRANSACTIONDATAMIN* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (ICE_TRANSACTIONDATAMIN*)pListTuple->GetNext (pos);
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

LONG CuDlgIpmIcePageTransaction::OnGetData (WPARAM wParam, LPARAM lParam)
{
	ICE_TRANSACTIONDATAMIN* p = NULL;
	CaIpmPropertyDataIcePageTransaction* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIcePageTransaction ();
		CTypedPtrList<CPtrList, ICE_TRANSACTIONDATAMIN*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			ICE_TRANSACTIONDATAMIN* pItem = (ICE_TRANSACTIONDATAMIN*)m_cListCtrl.GetItemData(i);
			if (!pItem)
				continue;
			p = new ICE_TRANSACTIONDATAMIN;
			memcpy (p, pItem, sizeof(ICE_TRANSACTIONDATAMIN));
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

void CuDlgIpmIcePageTransaction::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}

void CuDlgIpmIcePageTransaction::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	ICE_TRANSACTIONDATAMIN* p = (ICE_TRANSACTIONDATAMIN*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (p != NULL)
		delete p;
	*pResult = 0;
}

void CuDlgIpmIcePageTransaction::AddItem (LPCTSTR lpszItem, ICE_TRANSACTIONDATAMIN* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgIpmIcePageTransaction::DisplayItems()
{
	ICE_TRANSACTIONDATAMIN* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (ICE_TRANSACTIONDATAMIN*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, (LPCTSTR)p->trans_key);
		m_cListCtrl.SetItemText (i, 1, (LPCTSTR)p->name);
		m_cListCtrl.SetItemText (i, 2, (LPCTSTR)p->connect);
		m_cListCtrl.SetItemText (i, 3, (LPCTSTR)p->owner);
	}
}

void CuDlgIpmIcePageTransaction::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

void CuDlgIpmIcePageTransaction::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

long CuDlgIpmIcePageTransaction::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}

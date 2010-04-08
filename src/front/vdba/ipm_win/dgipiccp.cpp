/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipiccp.cpp, Implementation file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Ice Cache Page
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipiccp.h"
#include "ipmicdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgIpmIcePageCachePage::CuDlgIpmIcePageCachePage(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmIcePageCachePage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmIcePageCachePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmIcePageCachePage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIcePageCachePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIcePageCachePage, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmIcePageCachePage)
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
// CuDlgIpmIcePageCachePage message handlers

void CuDlgIpmIcePageCachePage::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmIcePageCachePage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);
	m_ImageList.Create(IDB_TM_ICE_FILEINFO_ALL, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[8] =
	{
		{IDS_TC_NAME,      100, LVCFMT_LEFT, FALSE},
		{IDS_TC_LOCATION,  100, LVCFMT_LEFT, FALSE},
		{IDS_TC_REQUESTER, 100, LVCFMT_LEFT, FALSE},
		{IDS_TC_COUNTER,   100, LVCFMT_LEFT, FALSE},
		{IDS_TC_TIME_OUT,  100, LVCFMT_RIGHT,FALSE},
		{IDS_TC_STATUS,    100, LVCFMT_LEFT, FALSE},
		{IDS_TC_REQUEST,   100, LVCFMT_LEFT, FALSE},
		{IDS_TC_EXIT_FILE, 150, LVCFMT_LEFT, TRUE }
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, 8);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmIcePageCachePage::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmIcePageCachePage::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	ICE_CACHEINFODATAMIN* pData;
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
		ICE_CACHEINFODATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_ICE_FILEINFO, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (ICE_CACHEINFODATAMIN*)listInfoStruct.RemoveHead();
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
		delete (ICE_CACHEINFODATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmIcePageCachePage::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIcePageCachePage")) == 0);
	CTypedPtrList<CPtrList, ICE_CACHEINFODATAMIN*>* pListTuple;
	pListTuple = (CTypedPtrList<CPtrList, ICE_CACHEINFODATAMIN*>*)lParam;

	ICE_CACHEINFODATAMIN* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (ICE_CACHEINFODATAMIN*)pListTuple->GetNext (pos);
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

LONG CuDlgIpmIcePageCachePage::OnGetData (WPARAM wParam, LPARAM lParam)
{
	ICE_CACHEINFODATAMIN* p = NULL;
	CaIpmPropertyDataIcePageCachePage* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIcePageCachePage ();
		CTypedPtrList<CPtrList, ICE_CACHEINFODATAMIN*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			ICE_CACHEINFODATAMIN* pItem = (ICE_CACHEINFODATAMIN*)m_cListCtrl.GetItemData(i);
			if (!pItem)
				continue;
			p = new ICE_CACHEINFODATAMIN;
			memcpy (p, pItem, sizeof(ICE_CACHEINFODATAMIN));
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

void CuDlgIpmIcePageCachePage::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	ASSERT (pNMHDR->code == NM_DBLCLK);
	
	// Get the selected item
	ASSERT (m_cListCtrl.GetSelectedCount() == 1);
	if (m_cListCtrl.GetSelectedCount() != 1)
		return;
	int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
	ASSERT (selItemId != -1);
	if (selItemId == -1)
		return;
	
	if (selItemId != -1) 
	{
		void* pFastStruct = (void*)m_cListCtrl.GetItemData(selItemId);
		if (!pFastStruct) 
		{
			AfxMessageBox (IDS_E_DIRECT_ACCESS);//"Cannot have direct access after load"
			return;
		}
	
		CTypedPtrList<CObList, CuIpmTreeFastItem*>  ipmTreeFastItemList;
	
		// tree organization reflected in FastItemList
		if (IpmCurSelHasStaticChildren(this))
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_ICE_FILEINFO));   // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_ICE_FILEINFO, pFastStruct));
	
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmIcePageCachePage::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	ICE_CACHEINFODATAMIN* p = (ICE_CACHEINFODATAMIN*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (p != NULL)
		delete p;
	*pResult = 0;
}

void CuDlgIpmIcePageCachePage::AddItem (LPCTSTR lpszItem, ICE_CACHEINFODATAMIN* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgIpmIcePageCachePage::DisplayItems()
{
	CString strItem;
	ICE_CACHEINFODATAMIN* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (ICE_CACHEINFODATAMIN*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, (LPCTSTR)p->name);
		m_cListCtrl.SetItemText (i, 1, (LPCTSTR)p->loc_name);
		m_cListCtrl.SetItemText (i, 2, (LPCTSTR)p->owner);
		strItem.Format (_T("%d"), p->counter);
		m_cListCtrl.SetItemText (i, 3, strItem);
		strItem.Format (_T("%d s"), p->itimeout);
		m_cListCtrl.SetItemText (i, 4, strItem);
		m_cListCtrl.SetItemText (i, 5, (LPCTSTR)p->status);
		strItem.Format (_T("%d"), p->req_count);
		m_cListCtrl.SetItemText (i, 6, strItem);
		m_cListCtrl.SetCheck (i, 7, (p->iexist == 1)? TRUE: FALSE);
		m_cListCtrl.EnableCheck (i, 7, FALSE);
	}
}

void CuDlgIpmIcePageCachePage::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

void CuDlgIpmIcePageCachePage::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

long CuDlgIpmIcePageCachePage::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}

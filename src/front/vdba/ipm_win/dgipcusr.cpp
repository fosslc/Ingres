/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipcusr.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Ice Connected User Page
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 17-Jan-2000 (uk$so01)
**    BUG #99938, Add the image to differentiate the type of Actived Database.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipcusr.h"
#include "ipmicdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgIpmIcePageConnectedUser::CuDlgIpmIcePageConnectedUser(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmIcePageConnectedUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmIcePageConnectedUser)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmIcePageConnectedUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIcePageConnectedUser)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIcePageConnectedUser, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmIcePageConnectedUser)
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
// CuDlgIpmIcePageConnectedUser message handlers

void CuDlgIpmIcePageConnectedUser::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmIcePageConnectedUser::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create(IDB_TM_ICE_CONNECTED_USER, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[4] =
	{
		{IDS_TC_NAME_COOKIES,   150,  LVCFMT_LEFT, FALSE},
		{IDS_TC_ICE_USER,       100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_ACTIVE_SESSION, 150,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TIME_OUT,        80,  LVCFMT_RIGHT,FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, 4);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmIcePageConnectedUser::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmIcePageConnectedUser::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	ICE_CONNECTED_USERDATAMIN* pData;
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
		ICE_CONNECTED_USERDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_ICE_CONNECTED_USER, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (ICE_CONNECTED_USERDATAMIN*)listInfoStruct.RemoveHead();
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
		delete (ICE_CONNECTED_USERDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmIcePageConnectedUser::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIcePageConnectedUser")) == 0);
	CTypedPtrList<CPtrList, ICE_CONNECTED_USERDATAMIN*>* pListTuple;
	pListTuple = (CTypedPtrList<CPtrList, ICE_CONNECTED_USERDATAMIN*>*)lParam;

	ICE_CONNECTED_USERDATAMIN* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (ICE_CONNECTED_USERDATAMIN*)pListTuple->GetNext (pos);
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

LONG CuDlgIpmIcePageConnectedUser::OnGetData (WPARAM wParam, LPARAM lParam)
{
	ICE_CONNECTED_USERDATAMIN* p = NULL;
	CaIpmPropertyDataIcePageConnectedUser* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIcePageConnectedUser ();
		CTypedPtrList<CPtrList, ICE_CONNECTED_USERDATAMIN*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			ICE_CONNECTED_USERDATAMIN* pItem = (ICE_CONNECTED_USERDATAMIN*)m_cListCtrl.GetItemData(i);
			if (!pItem)
				continue;
			p = new ICE_CONNECTED_USERDATAMIN;
			memcpy (p, pItem, sizeof(ICE_CONNECTED_USERDATAMIN));
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

void CuDlgIpmIcePageConnectedUser::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_ICE_CONNECTED_USER));   // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_ICE_CONNECTED_USER, pFastStruct));
	
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmIcePageConnectedUser::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	ICE_CONNECTED_USERDATAMIN* p = (ICE_CONNECTED_USERDATAMIN*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (p != NULL)
		delete p;
	*pResult = 0;
}

void CuDlgIpmIcePageConnectedUser::AddItem (LPCTSTR lpszItem, ICE_CONNECTED_USERDATAMIN* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgIpmIcePageConnectedUser::DisplayItems()
{
	CString strItem;
	ICE_CONNECTED_USERDATAMIN* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (ICE_CONNECTED_USERDATAMIN*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, (LPCTSTR)p->name);
		m_cListCtrl.SetItemText (i, 1, (LPCTSTR)p->inguser);
		strItem.Format (_T("%ld"), p->req_count);
		m_cListCtrl.SetItemText (i, 2, strItem);
		strItem.Format (_T("%ld s"), p->timeout);
		m_cListCtrl.SetItemText (i, 3, strItem);
	}
}


void CuDlgIpmIcePageConnectedUser::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

void CuDlgIpmIcePageConnectedUser::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

long CuDlgIpmIcePageConnectedUser::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}

/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiplock.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Locks page.
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgiplock.h"
#include "ipmprdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 7


static void GetDisplayInfo (
	LPLOCKDATAMIN pData, 
	CString& strID, 
	CString& strRQ, 
	CString& strState, 
	CString& strLockType, 
	CString& strDB, 
	CString& strTable, 
	CString& strPage)
{
	TCHAR tchszBuffer [80];

	strID = FormatHexaL (pData->lock_id, tchszBuffer);
	strRQ = pData->lock_request_mode;
	strState = pData->lock_state;
	strLockType = FindResourceString(pData->resource_type); 
	strDB = pData->res_database_name;
	strTable = IPM_GetLockTableNameStr(pData,tchszBuffer,sizeof(tchszBuffer));
	strPage  = IPM_GetLockPageNameStr (pData,tchszBuffer); 
}

CuDlgIpmPageLocks::CuDlgIpmPageLocks(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageLocks::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageLocks)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageLocks::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLocks)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageLocks::AddLock (
	LPCTSTR strID,
	LPCTSTR strRQ,
	LPCTSTR strState,
	LPCTSTR strLockType,
	LPCTSTR strDB,
	LPCTSTR strTable,
	LPCTSTR strPage,
	void* pStruct)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the ID
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strID, 0);
	//
	// Add the RQ
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strRQ);
	//
	// Add the State
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strState);
	//
	// Add the Type
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strLockType);
	//
	// Add the DB
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strDB);
	//
	// Add the Table
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strTable);
	//
	// Add the Page
	m_cListCtrl.SetItemText (index, 6, (LPTSTR)strPage);

	// Store the fast access structure
	if (pStruct) 
	{
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	}
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLocks, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageLocks)
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
// CuDlgIpmPageLocks message handlers

void CuDlgIpmPageLocks::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageLocks::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_LOCKID,    76,  LVCFMT_LEFT, FALSE},
		{IDS_TC_RQ,        35,  LVCFMT_LEFT, FALSE},
		{IDS_TC_STATE,     80,  LVCFMT_LEFT, FALSE},
		{IDS_TC_LOCK_TYPE, 90,  LVCFMT_LEFT, FALSE},
		{IDS_TC_DATABASE,  65,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TABLE,     80,  LVCFMT_LEFT, FALSE},
		{IDS_TC_PAGE,     100,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);
	m_ImageList.Create(IDB_TM_LOCK, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageLocks::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageLocks::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	LOCKDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strID, strRQ, strState, strLockType, strDB, strTable, strPage;

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
		LOCKDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOCK, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (LOCKDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strID, strRQ, strState, strLockType, strDB, strTable, strPage);
				AddLock (strID, strRQ, strState, strLockType, strDB, strTable, strPage, pData);
			}
		}
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
		delete (LOCKDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageLocks::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageLocks")) == 0);
	CTypedPtrList<CObList, CuDataTupleLock*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleLock*>*)lParam;
	CuDataTupleLock* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleLock*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddLock (
				pObj->m_strID, 
				pObj->m_strRQ, 
				pObj->m_strState, 
				pObj->m_strLockType, 
				pObj->m_strDB, 
				pObj->m_strTable,
				pObj->m_strPage,
				NULL);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLocks::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLocks* pData = NULL;
	CString strID ;
	CString strRQ;
	CString strState;
	CString strLockType;
	CString strDB;
	CString strTable;
	CString strPage;
	try
	{
		CuDataTupleLock* pObj;
		pData = new CuIpmPropertyDataPageLocks ();
		CTypedPtrList<CObList, CuDataTupleLock*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strID = m_cListCtrl.GetItemText (i, 0);
			strRQ = m_cListCtrl.GetItemText (i, 1); 
			strState = m_cListCtrl.GetItemText (i, 2);
			strLockType = m_cListCtrl.GetItemText (i, 3);
			strDB = m_cListCtrl.GetItemText (i, 4);
			strTable = m_cListCtrl.GetItemText (i, 5);
			strPage = m_cListCtrl.GetItemText (i, 6);
			pObj = new CuDataTupleLock (strID, strRQ, strState, strLockType, strDB, strTable, strPage);
			listTuple.AddTail (pObj);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageLocks::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	ASSERT (pNMHDR->code == NM_DBLCLK);
	
	// Get the selected item
	ASSERT (m_cListCtrl.GetSelectedCount() == 1);
	int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
	ASSERT (selItemId != -1);
	
	if (selItemId != -1) {
		void* pFastStruct = (void*)m_cListCtrl.GetItemData(selItemId);
		if (!pFastStruct) {
			AfxMessageBox (IDS_E_DIRECT_ACCESS);//"Cannot have direct access after load"
			return;
		}
		
		CTypedPtrList<CObList, CuIpmTreeFastItem*> ipmTreeFastItemList;
		
		// tree organization reflected in FastItemList
		if (IpmCurSelHasStaticChildren(this))
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_LOCK));  // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_LOCK, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageLocks::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageLocks::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageLocks::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageLocks::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
